#include "QtShaderCanvas.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QImage>
#include <QElapsedTimer>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QMouseEvent>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

// Vertex Shader for fullscreen quad
static const char* vertexShaderSource = R"(#version 330 core
layout(location = 0) in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// Fallback default shader
static const char* fallbackFragmentShaderSource = R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord / iResolution.xy;

    // Time-varying pixel color
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0.0, 2.0, 4.0));

    // Output to screen
    fragColor = vec4(col, 1.0);
}
)";

// Helper function to wrap user shader code with ShaderToy boilerplate
static QString wrapShader(const QString &sourceCode) {
    if (!sourceCode.contains("mainImage")) {
        return sourceCode;
    }

    QString wrapped;
    wrapped += "#version 330 core\n";
    wrapped += "#ifdef GL_ES\n";
    wrapped += "precision highp float;\n";
    wrapped += "#endif\n\n";

    wrapped += "uniform vec3 iResolution;\n";
    wrapped += "uniform float iTime;\n";
    wrapped += "uniform float iTimeDelta;\n";
    wrapped += "uniform int iFrame;\n";
    wrapped += "uniform vec4 iMouse;\n";
    wrapped += "uniform vec4 iDate;\n";
    wrapped += "uniform sampler2D iChannel0;\n";
    wrapped += "uniform sampler2D iChannel1;\n";
    wrapped += "uniform sampler2D iChannel2;\n";
    wrapped += "uniform sampler2D iChannel3;\n";
    wrapped += "uniform vec3 iChannelResolution[4];\n\n";

    // Ghostty cursor uniforms support
    wrapped += "uniform vec4 iCurrentCursor;\n";
    wrapped += "uniform vec4 iPreviousCursor;\n";
    wrapped += "uniform float iTimeCursorChange;\n";
    wrapped += "uniform vec4 iCurrentCursorColor;\n";
    wrapped += "uniform vec4 iPreviousCursorColor;\n\n";

    wrapped += "#line 1\n";
    wrapped += sourceCode;
    wrapped += "\n\n";

    wrapped += "out vec4 fragColor_out;\n";
    wrapped += "void main() {\n";
    wrapped += "    mainImage(fragColor_out, gl_FragCoord.xy);\n";
    wrapped += "}\n";

    return wrapped;
}

// Private implementation class
class QtShaderCanvasPrivate : public QOpenGLFunctions {
public:
    QtShaderCanvas* q_ptr;
    Q_DECLARE_PUBLIC(QtShaderCanvas)

    QOpenGLShaderProgram *shaderProgram = nullptr;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;

    QFileSystemWatcher *fileWatcher = nullptr;
    QElapsedTimer elapsedTimer;
    QTimer *renderTimer = nullptr;

    float time = 0.0f;
    qint64 lastFrameTimeMs = 0;
    float timeDelta = 0.0f;
    int frameCount = 0;
    QVector4D mouse = QVector4D(0.0f, 0.0f, 0.0f, 0.0f);
    bool isPlaying = false;
    bool isMousePressed = false;
    QPointF lastClickPos = QPointF(0.0f, 0.0f);
    int fpsLimit = 60;
    bool hotReloadEnabled = true;

    QString shaderPath;
    QString shaderCode;
    QString shaderErrorString;

    QString backgroundImagePath;
    QOpenGLTexture *backgroundTexture = nullptr;

    QtShaderCanvasPrivate(QtShaderCanvas *q)
        : q_ptr(q)
        , vbo(QOpenGLBuffer::VertexBuffer)
    {}

    ~QtShaderCanvasPrivate() {
        q_ptr->makeCurrent();
        if (shaderProgram) {
            delete shaderProgram;
        }
        if (backgroundTexture) {
            backgroundTexture->destroy();
            delete backgroundTexture;
        }
        vbo.destroy();
        vao.destroy();
        q_ptr->doneCurrent();
    }

    bool compileProgram(const QString &fragCode);
    void updateTime();
    void updateTimerInterval();
    void onFileChanged(const QString &path);
};

bool QtShaderCanvasPrivate::compileProgram(const QString &fragCode) {
    QString wrappedFrag = wrapShader(fragCode);

    QOpenGLShaderProgram *newProgram = new QOpenGLShaderProgram(q_ptr);

    if (!newProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        shaderErrorString = "Vertex Shader Compile Error:\n" + newProgram->log();
        delete newProgram;
        Q_EMIT q_ptr->shaderError(shaderErrorString);
        return false;
    }

    if (!newProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, wrappedFrag)) {
        shaderErrorString = "Fragment Shader Compile Error:\n" + newProgram->log();
        delete newProgram;
        Q_EMIT q_ptr->shaderError(shaderErrorString);
        return false;
    }

    if (!newProgram->link()) {
        shaderErrorString = "Shader Program Link Error:\n" + newProgram->log();
        delete newProgram;
        Q_EMIT q_ptr->shaderError(shaderErrorString);
        return false;
    }

    // Context needs to be current to replace program
    q_ptr->makeCurrent();
    if (shaderProgram) {
        delete shaderProgram;
    }
    shaderProgram = newProgram;
    shaderCode = fragCode;
    shaderErrorString.clear();
    q_ptr->doneCurrent();

    return true;
}

void QtShaderCanvasPrivate::updateTime() {
    if (isPlaying) {
        qint64 currentElapsed = elapsedTimer.elapsed();
        timeDelta = (currentElapsed - lastFrameTimeMs) / 1000.0f;
        lastFrameTimeMs = currentElapsed;

        time += timeDelta;
        frameCount++;
        
        Q_EMIT q_ptr->timeChanged(time);
        Q_EMIT q_ptr->frameChanged(frameCount);
    } else {
        timeDelta = 0.0f;
    }
}

void QtShaderCanvasPrivate::updateTimerInterval() {
    if (!renderTimer) return;
    
    if (fpsLimit > 0) {
        renderTimer->setInterval(1000 / fpsLimit);
    } else {
        renderTimer->setInterval(0);
    }
}

void QtShaderCanvasPrivate::onFileChanged(const QString &path) {
    if (path == shaderPath) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString code = stream.readAll();
            file.close();

            if (compileProgram(code)) {
                q_ptr->update();
            }
        }

        // Re-add to watcher since some editors replace the file entirely
        if (fileWatcher) {
            fileWatcher->removePath(path);
            fileWatcher->addPath(path);
        }
    }
}

// Public Widget Class
QtShaderCanvas::QtShaderCanvas(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new QtShaderCanvasPrivate(this))
{
    Q_D(QtShaderCanvas);
    
    // Set update timer
    d->renderTimer = new QTimer(this);
    connect(d->renderTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    d->updateTimerInterval();
    
    // Enable mouse tracking to receive drag events
    setMouseTracking(true);
}

QtShaderCanvas::~QtShaderCanvas() = default;

bool QtShaderCanvas::loadShader(const QString &filePath) {
    Q_D(QtShaderCanvas);

    if (d->fileWatcher && !d->shaderPath.isEmpty()) {
        d->fileWatcher->removePath(d->shaderPath);
    }

    d->shaderPath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err = QString("Could not open file: %1").arg(filePath);
        d->shaderErrorString = err;
        Q_EMIT shaderError(err);
        return false;
    }

    QTextStream stream(&file);
    QString code = stream.readAll();
    file.close();

    bool success = loadShaderCode(code);
    if (success) {
        if (d->hotReloadEnabled) {
            if (!d->fileWatcher) {
                d->fileWatcher = new QFileSystemWatcher(this);
                connect(d->fileWatcher, &QFileSystemWatcher::fileChanged, this, [d](const QString &path) {
                    d->onFileChanged(path);
                });
            }
            d->fileWatcher->addPath(filePath);
        }
        // Restore filepath since loadShaderCode clears it
        d->shaderPath = filePath;
        Q_EMIT shaderLoaded(filePath);
    }
    return success;
}

bool QtShaderCanvas::loadShaderCode(const QString &sourceCode) {
    Q_D(QtShaderCanvas);

    if (d->fileWatcher && !d->shaderPath.isEmpty()) {
        d->fileWatcher->removePath(d->shaderPath);
        d->shaderPath.clear();
    }

    return d->compileProgram(sourceCode);
}

bool QtShaderCanvas::isPlaying() const {
    Q_D(const QtShaderCanvas);
    return d->isPlaying;
}

float QtShaderCanvas::time() const {
    Q_D(const QtShaderCanvas);
    return d->time;
}

int QtShaderCanvas::frame() const {
    Q_D(const QtShaderCanvas);
    return d->frameCount;
}

int QtShaderCanvas::fpsLimit() const {
    Q_D(const QtShaderCanvas);
    return d->fpsLimit;
}

bool QtShaderCanvas::isHotReloadEnabled() const {
    Q_D(const QtShaderCanvas);
    return d->hotReloadEnabled;
}

QString QtShaderCanvas::currentShaderPath() const {
    Q_D(const QtShaderCanvas);
    return d->shaderPath;
}

QString QtShaderCanvas::shaderErrorString() const {
    Q_D(const QtShaderCanvas);
    return d->shaderErrorString;
}

void QtShaderCanvas::play() {
    Q_D(QtShaderCanvas);
    if (d->isPlaying) return;

    d->isPlaying = true;
    if (d->frameCount == 0) {
        d->elapsedTimer.start();
        d->lastFrameTimeMs = 0;
    } else {
        d->lastFrameTimeMs = d->elapsedTimer.elapsed();
    }
    
    d->renderTimer->start();
    Q_EMIT playStateChanged(true);
    update();
}

void QtShaderCanvas::pause() {
    Q_D(QtShaderCanvas);
    if (!d->isPlaying) return;

    d->isPlaying = false;
    d->renderTimer->stop();
    Q_EMIT playStateChanged(false);
    update();
}

void QtShaderCanvas::stop() {
    Q_D(QtShaderCanvas);
    d->time = 0.0f;
    d->frameCount = 0;
    d->timeDelta = 0.0f;
    d->isPlaying = false;
    d->renderTimer->stop();
    
    Q_EMIT playStateChanged(false);
    Q_EMIT timeChanged(0.0f);
    Q_EMIT frameChanged(0);
    update();
}

void QtShaderCanvas::step() {
    Q_D(QtShaderCanvas);
    if (d->isPlaying) {
        pause();
    }
    
    d->frameCount++;
    d->timeDelta = 1.0f / 60.0f;
    d->time += d->timeDelta;
    
    Q_EMIT timeChanged(d->time);
    Q_EMIT frameChanged(d->frameCount);
    update();
}

void QtShaderCanvas::setFpsLimit(int fps) {
    Q_D(QtShaderCanvas);
    if (d->fpsLimit == fps) return;

    d->fpsLimit = fps;
    d->updateTimerInterval();
    Q_EMIT fpsLimitChanged(fps);
}

void QtShaderCanvas::setHotReloadEnabled(bool enabled) {
    Q_D(QtShaderCanvas);
    if (d->hotReloadEnabled == enabled) return;

    d->hotReloadEnabled = enabled;

    if (enabled && !d->shaderPath.isEmpty()) {
        if (!d->fileWatcher) {
            d->fileWatcher = new QFileSystemWatcher(this);
            connect(d->fileWatcher, &QFileSystemWatcher::fileChanged, this, [d](const QString &path) {
                d->onFileChanged(path);
            });
        }
        d->fileWatcher->addPath(d->shaderPath);
    } else if (!enabled && d->fileWatcher) {
        d->fileWatcher->removePaths(d->fileWatcher->files());
    }

    Q_EMIT hotReloadEnabledChanged(enabled);
}

void QtShaderCanvas::initializeGL() {
    Q_D(QtShaderCanvas);
    d->initializeOpenGLFunctions();

    d->glDisable(GL_DEPTH_TEST);
    d->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Setup Geometry (Fullscreen Quad VAO/VBO)
    d->vao.create();
    d->vao.bind();

    static const float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    d->vbo.create();
    d->vbo.bind();
    d->vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    d->vbo.allocate(vertices, sizeof(vertices));

    d->glEnableVertexAttribArray(0);
    d->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    d->vao.release();
    d->vbo.release();

    // Compile default shader if none compiled yet
    if (!d->shaderProgram) {
        d->compileProgram(fallbackFragmentShaderSource);
    }
    
    // Automatically play by default
    play();
}

void QtShaderCanvas::resizeGL(int w, int h) {
    Q_D(QtShaderCanvas);
    // w and h are in physical pixels
    d->glViewport(0, 0, w, h);
}

void QtShaderCanvas::paintGL() {
    Q_D(QtShaderCanvas);

    // Clear buffer
    d->glClear(GL_COLOR_BUFFER_BIT);

    // Update time values
    d->updateTime();

    if (d->shaderProgram && d->shaderProgram->isLinked()) {
        d->shaderProgram->bind();

        // 1. iResolution
        float ratio = devicePixelRatioF();
        QVector3D resolution(width() * ratio, height() * ratio, ratio);
        d->shaderProgram->setUniformValue("iResolution", resolution);

        // 2. iTime
        d->shaderProgram->setUniformValue("iTime", d->time);

        // 3. iTimeDelta
        d->shaderProgram->setUniformValue("iTimeDelta", d->timeDelta);

        // 4. iFrame
        d->shaderProgram->setUniformValue("iFrame", d->frameCount);

        // 5. iMouse
        d->shaderProgram->setUniformValue("iMouse", d->mouse);

        // 6. iDate
        QDateTime now = QDateTime::currentDateTime();
        QDate date = now.date();
        QTime qtime = now.time();
        float timeSec = qtime.hour() * 3600.0f + qtime.minute() * 60.0f + qtime.second() + qtime.msec() / 1000.0f;
        d->shaderProgram->setUniformValue("iDate", QVector4D(date.year(), date.month() - 1, date.day(), timeSec));

        // 6b. Ghostty cursor uniforms (default simulated cursor)
        d->shaderProgram->setUniformValue("iCurrentCursor", QVector4D(width() * ratio / 2.0f, height() * ratio / 2.0f, 80.0f * ratio, 40.0f * ratio));
        d->shaderProgram->setUniformValue("iPreviousCursor", QVector4D(width() * ratio / 2.0f - 50.0f, height() * ratio / 2.0f - 20.0f, 80.0f * ratio, 40.0f * ratio));
        d->shaderProgram->setUniformValue("iTimeCursorChange", d->time - 0.5f);
        d->shaderProgram->setUniformValue("iCurrentCursorColor", QVector4D(0.4f, 0.7f, 1.0f, 1.0f));
        d->shaderProgram->setUniformValue("iPreviousCursorColor", QVector4D(0.4f, 0.7f, 1.0f, 1.0f));

        // 7. iChannel0-3 (Texture units) and dummy resolution array to prevent compilation errors
        d->shaderProgram->setUniformValue("iChannel0", 0);
        d->shaderProgram->setUniformValue("iChannel1", 1);
        d->shaderProgram->setUniformValue("iChannel2", 2);
        d->shaderProgram->setUniformValue("iChannel3", 3);

        // Load background texture if path is set and texture is not loaded yet
        if (!d->backgroundImagePath.isEmpty() && !d->backgroundTexture) {
            QImage img(d->backgroundImagePath);
            if (!img.isNull()) {
                d->backgroundTexture = new QOpenGLTexture(img.mirrored());
                d->backgroundTexture->setMinificationFilter(QOpenGLTexture::Linear);
                d->backgroundTexture->setMagnificationFilter(QOpenGLTexture::Linear);
                d->backgroundTexture->setWrapMode(QOpenGLTexture::Repeat);
            }
        }

        if (d->backgroundTexture && d->backgroundTexture->isCreated()) {
            d->backgroundTexture->bind(0);
            d->backgroundTexture->bind(1);
            d->backgroundTexture->bind(2);
            d->backgroundTexture->bind(3);

            float w = d->backgroundTexture->width();
            float h = d->backgroundTexture->height();
            QVector3D res[4] = {
                QVector3D(w, h, 1.0f),
                QVector3D(w, h, 1.0f),
                QVector3D(w, h, 1.0f),
                QVector3D(w, h, 1.0f)
            };
            d->shaderProgram->setUniformValueArray("iChannelResolution", res, 4);
        } else {
            QVector3D dummyRes[4] = {
                QVector3D(0.0f, 0.0f, 0.0f),
                QVector3D(0.0f, 0.0f, 0.0f),
                QVector3D(0.0f, 0.0f, 0.0f),
                QVector3D(0.0f, 0.0f, 0.0f)
            };
            d->shaderProgram->setUniformValueArray("iChannelResolution", dummyRes, 4);
        }

        // Bind VAO & Draw fullscreen quad
        d->vao.bind();
        d->glDrawArrays(GL_TRIANGLES, 0, 6);
        d->vao.release();

        d->shaderProgram->release();
    }
}

void QtShaderCanvas::mousePressEvent(QMouseEvent *event) {
    Q_D(QtShaderCanvas);
    if (event->button() == Qt::LeftButton) {
        d->isMousePressed = true;
        float ratio = devicePixelRatioF();
        float x = event->position().x() * ratio;
        float y = (height() - event->position().y()) * ratio;

        d->mouse.setX(x);
        d->mouse.setY(y);
        d->mouse.setZ(x);
        d->mouse.setW(y);

        d->lastClickPos = QPointF(x, y);
    }
    QOpenGLWidget::mousePressEvent(event);
}

void QtShaderCanvas::mouseMoveEvent(QMouseEvent *event) {
    Q_D(QtShaderCanvas);
    if (d->isMousePressed) {
        float ratio = devicePixelRatioF();
        float x = event->position().x() * ratio;
        float y = (height() - event->position().y()) * ratio;

        d->mouse.setX(x);
        d->mouse.setY(y);
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void QtShaderCanvas::mouseReleaseEvent(QMouseEvent *event) {
    Q_D(QtShaderCanvas);
    if (event->button() == Qt::LeftButton) {
        d->isMousePressed = false;
        // Invert Z & W to signal button release while retaining position of last mouse down
        d->mouse.setZ(-qAbs(d->lastClickPos.x()));
        d->mouse.setW(-qAbs(d->lastClickPos.y()));
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void QtShaderCanvas::setBackgroundImage(const QString &filePath) {
    Q_D(QtShaderCanvas);
    if (d->backgroundImagePath == filePath) return;

    d->backgroundImagePath = filePath;

    // Clean up previous texture if it exists. Re-creation will happen on next paintGL().
    makeCurrent();
    if (d->backgroundTexture) {
        d->backgroundTexture->destroy();
        delete d->backgroundTexture;
        d->backgroundTexture = nullptr;
    }
    doneCurrent();

    update();
}

QString QtShaderCanvas::backgroundImagePath() const {
    Q_D(const QtShaderCanvas);
    return d->backgroundImagePath;
}

