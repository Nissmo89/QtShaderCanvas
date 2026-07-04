#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QImage>
#include <QOpenGLFunctions>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <iostream>

// Exact replica of wrapShader from QtShaderCanvas.cpp
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

static const char* vertexShaderSource = R"(#version 330 core
layout(location = 0) in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

int main(int argc, char *argv[]) {
    // Force offscreen platform to run headlessly
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);

    // Create an OpenGL context
    QOpenGLContext context;
    if (!context.create()) {
        std::cerr << "Error: Failed to create QOpenGLContext!" << std::endl;
        return 1;
    }

    // Create a dummy offscreen surface
    QOffscreenSurface surface;
    surface.create();
    if (!surface.isValid()) {
        std::cerr << "Error: Failed to create QOffscreenSurface!" << std::endl;
        return 1;
    }

    // Make the context current
    if (!context.makeCurrent(&surface)) {
        std::cerr << "Error: Failed to make OpenGL context current!" << std::endl;
        return 1;
    }

    QOpenGLFunctions *funcs = context.functions();
    funcs->initializeOpenGLFunctions();

    // Set up geometry for fullscreen quad
    static const float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));

    QOpenGLVertexArrayObject vao;
    vao.create();
    vao.bind();

    // Load background texture if exists
    QOpenGLTexture *backgroundTexture = nullptr;
    QString bgPath = "06. Green Lush.jpg";
    if (!QFile::exists(bgPath)) {
        bgPath = "../06. Green Lush.jpg";
    }
    if (!QFile::exists(bgPath)) {
        bgPath = "/home/nord/code_base/QtShaderCanvas/06. Green Lush.jpg";
    }
    QImage img(bgPath);
    if (!img.isNull()) {
        backgroundTexture = new QOpenGLTexture(img.mirrored());
        backgroundTexture->setMinificationFilter(QOpenGLTexture::Linear);
        backgroundTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        backgroundTexture->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        std::cerr << "Warning: Could not load background image: " << bgPath.toStdString() << std::endl;
    }

    // Setup offscreen framebuffer
    QOpenGLFramebufferObject fbo(1024, 600);
    fbo.bind();
    funcs->glViewport(0, 0, 1024, 600);

    QDir dir("GLSL-Shaders");
    if (!dir.exists()) {
        std::cerr << "Error: GLSL-Shaders directory does not exist!" << std::endl;
        return 1;
    }

    QStringList filters;
    filters << "*.glsl" << "*.frag";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::NoSymLinks, QDir::Name);

    std::cout << "--- SHADER CHECKER START ---" << std::endl;
    std::cout << "Total shaders found: " << files.size() << std::endl;

    for (const QFileInfo &fileInfo : files) {
        QString path = fileInfo.absoluteFilePath();
        std::cout << "[FILE] " << fileInfo.fileName().toStdString() << std::endl;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::cout << "[RESULT] FAILED" << std::endl;
            std::cout << "[ERROR_BEGIN]\nCould not open file\n[ERROR_END]" << std::endl;
            std::cout << "[SEPARATOR]" << std::endl;
            continue;
        }

        QTextStream stream(&file);
        QString code = stream.readAll();
        file.close();

        QString wrappedFrag = wrapShader(code);

        QOpenGLShaderProgram program;
        bool ok = true;
        QString errorLog;

        if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
            ok = false;
            errorLog = "Vertex Shader Error: " + program.log();
        } else if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, wrappedFrag)) {
            ok = false;
            errorLog = "Fragment Shader Error: " + program.log();
        } else if (!program.link()) {
            ok = false;
            errorLog = "Link Error: " + program.log();
        }

        if (ok) {
            std::cout << "[RESULT] SUCCESS" << std::endl;

            // Render a test frame to the FBO
            funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            program.bind();

            vao.bind();
            vbo.bind();
            program.enableAttributeArray(0);
            program.setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(float));

            // Bind background texture to units 0, 1, 2, 3
            if (backgroundTexture && backgroundTexture->isCreated()) {
                backgroundTexture->bind(0);
                backgroundTexture->bind(1);
                backgroundTexture->bind(2);
                backgroundTexture->bind(3);

                float texW = backgroundTexture->width();
                float texH = backgroundTexture->height();
                QVector3D res[4] = {
                    QVector3D(texW, texH, 1.0f),
                    QVector3D(texW, texH, 1.0f),
                    QVector3D(texW, texH, 1.0f),
                    QVector3D(texW, texH, 1.0f)
                };
                program.setUniformValueArray("iChannelResolution", res, 4);
            } else {
                QVector3D dummyRes[4] = {
                    QVector3D(0.0f, 0.0f, 0.0f),
                    QVector3D(0.0f, 0.0f, 0.0f),
                    QVector3D(0.0f, 0.0f, 0.0f),
                    QVector3D(0.0f, 0.0f, 0.0f)
                };
                program.setUniformValueArray("iChannelResolution", dummyRes, 4);
            }

            program.setUniformValue("iResolution", QVector3D(1024.0f, 600.0f, 1.0f));
            program.setUniformValue("iTime", 2.5f);
            program.setUniformValue("iTimeDelta", 0.016f);
            program.setUniformValue("iFrame", 150);
            program.setUniformValue("iMouse", QVector4D(512.0f, 300.0f, 0.0f, 0.0f));

            QDateTime now = QDateTime::currentDateTime();
            QDate date = now.date();
            QTime qtime = now.time();
            float timeSec = qtime.hour() * 3600.0f + qtime.minute() * 60.0f + qtime.second() + qtime.msec() / 1000.0f;
            program.setUniformValue("iDate", QVector4D(date.year(), date.month() - 1, date.day(), timeSec));

            program.setUniformValue("iChannel0", 0);
            program.setUniformValue("iChannel1", 1);
            program.setUniformValue("iChannel2", 2);
            program.setUniformValue("iChannel3", 3);

            // Set Ghostty simulated values
            program.setUniformValue("iCurrentCursor", QVector4D(512.0f, 300.0f, 100.0f, 50.0f));
            program.setUniformValue("iPreviousCursor", QVector4D(400.0f, 300.0f, 100.0f, 50.0f));
            program.setUniformValue("iTimeCursorChange", 2.0f);
            program.setUniformValue("iCurrentCursorColor", QVector4D(0.4f, 0.7f, 1.0f, 1.0f));
            program.setUniformValue("iPreviousCursorColor", QVector4D(0.4f, 0.7f, 1.0f, 1.0f));

            funcs->glDrawArrays(GL_TRIANGLES, 0, 6);

            // Save visual screenshot
            QImage screenshot = fbo.toImage();
            QDir().mkpath("visual_checks");
            QString visualCheckPath = QString("visual_checks/%1.png").arg(fileInfo.baseName());
            if (screenshot.save(visualCheckPath)) {
                std::cout << "[VISUAL_CHECK] Saved to " << visualCheckPath.toStdString() << std::endl;
            } else {
                std::cout << "[VISUAL_CHECK] FAILED to save" << std::endl;
            }

            vao.release();
            vbo.release();
            program.release();
        } else {
            std::cout << "[RESULT] FAILED" << std::endl;
            std::cout << "[ERROR_BEGIN]\n" << errorLog.toStdString() << "\n[ERROR_END]" << std::endl;
        }
        std::cout << "[SEPARATOR]" << std::endl;
    }

    // Cleanup resources
    if (backgroundTexture) {
        backgroundTexture->destroy();
        delete backgroundTexture;
    }
    vbo.destroy();
    vao.destroy();

    std::cout << "--- SHADER CHECKER END ---" << std::endl;
    return 0;
}
