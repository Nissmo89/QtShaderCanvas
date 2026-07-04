#ifndef QTSHADERCANVAS_H
#define QTSHADERCANVAS_H

#include <QOpenGLWidget>
#include <QScopedPointer>

class QtShaderCanvasPrivate;

class QtShaderCanvas : public QOpenGLWidget {
    Q_OBJECT
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playStateChanged)
    Q_PROPERTY(float time READ time NOTIFY timeChanged)
    Q_PROPERTY(int frame READ frame NOTIFY frameChanged)
    Q_PROPERTY(int fpsLimit READ fpsLimit WRITE setFpsLimit NOTIFY fpsLimitChanged)
    Q_PROPERTY(bool hotReloadEnabled READ isHotReloadEnabled WRITE setHotReloadEnabled NOTIFY hotReloadEnabledChanged)
    Q_PROPERTY(QString shaderPath READ currentShaderPath NOTIFY shaderLoaded)

public:
    explicit QtShaderCanvas(QWidget *parent = nullptr);
    ~QtShaderCanvas() override;

    // Load shaders
    bool loadShader(const QString &filePath);
    bool loadShaderCode(const QString &sourceCode);

    // Getters
    bool isPlaying() const;
    float time() const;
    int frame() const;
    int fpsLimit() const;
    bool isHotReloadEnabled() const;
    QString currentShaderPath() const;
    QString shaderErrorString() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void step();
    void setFpsLimit(int fps);
    void setHotReloadEnabled(bool enabled);

Q_SIGNALS:
    void shaderLoaded(const QString &filePath);
    void shaderError(const QString &errorMessage);
    void timeChanged(float time);
    void frameChanged(int frame);
    void playStateChanged(bool playing);
    void fpsLimitChanged(int fps);
    void hotReloadEnabledChanged(bool enabled);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Q_DISABLE_COPY(QtShaderCanvas)
    QScopedPointer<QtShaderCanvasPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QtShaderCanvas)
};

#endif // QTSHADERCANVAS_H
