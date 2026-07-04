#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

class QtShaderCanvas;
class QComboBox;
class QPushButton;
class QCheckBox;
class QLabel;
class QTextEdit;
class QSlider;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private Q_SLOTS:
    void onShaderSelected(int index);
    void onLoadFileClicked();
    void onPlayStateChanged(bool playing);
    void onShaderLoaded(const QString &path);
    void onShaderError(const QString &error);
    void onFpsLimitChanged(int value);
    void updateStats();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();
    void applyAesthetics();
    void updateFpsLabel(int fps);

    QtShaderCanvas *m_canvas = nullptr;
    
    // UI elements
    QComboBox *m_shaderSelector = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_stepButton = nullptr;
    QPushButton *m_openButton = nullptr;
    QCheckBox *m_hotReloadToggle = nullptr;
    QSlider *m_fpsSlider = nullptr;
    QLabel *m_fpsSliderLabel = nullptr;
    
    // Stats labels
    QLabel *m_timeLabel = nullptr;
    QLabel *m_frameLabel = nullptr;
    QLabel *m_fpsCounterLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    
    // Error log
    QTextEdit *m_errorConsole = nullptr;

    // Timer for stats updates
    QTimer *m_statsTimer = nullptr;
    
    // FPS counter calculation variables
    int m_lastFrameCount = 0;
    qint64 m_lastTimeMs = 0;
};

#endif // MAINWINDOW_H
