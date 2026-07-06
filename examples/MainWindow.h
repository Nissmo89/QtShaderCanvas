#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

class QtShaderCanvas;
class CodeEditor;
class QComboBox;
class QPushButton;
class QCheckBox;
class QLabel;
class QTextEdit;
class QSlider;
class QTimer;
class QAction;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private Q_SLOTS:
    void onEditorTextChanged();
    void triggerCompile();
    void onShaderLoaded(const QString &path);
    void onShaderError(const QString &error);
    void onPlayStateChanged(bool playing);
    void onFpsLimitChanged(int value);
    void updateStats();
    
    // File operations
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    
    // Shader examples load
    void loadShaderIntoEditor(const QString &filePath);
    
    // Channel selections
    void onChannel0Changed(int index);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();
    void setupMenus();
    void applyAesthetics();
    void updateFpsLabel(int fps);

    // Canvas & Editor
    QtShaderCanvas *m_canvas = nullptr;
    CodeEditor *m_editor = nullptr;
    
    // Errors / Log
    QTextEdit *m_errorConsole = nullptr;
    
    // Uniform Inspector elements
    QLabel *m_inspectTime = nullptr;
    QLabel *m_inspectFrame = nullptr;
    QLabel *m_inspectResolution = nullptr;
    QLabel *m_inspectMouse = nullptr;
    QLabel *m_inspectDate = nullptr;
    QComboBox *m_channel0Selector = nullptr;
    
    // Playback Controls
    QPushButton *m_playButton = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_stepButton = nullptr;
    
    // Config controls in Uniform Inspector
    QCheckBox *m_hotReloadToggle = nullptr;
    QSlider *m_fpsSlider = nullptr;
    QLabel *m_fpsSliderLabel = nullptr;

    // Timers
    QTimer *m_statsTimer = nullptr;
    QTimer *m_compileTimer = nullptr;
    
    // State variables
    QString m_currentFilePath;
    int m_lastFrameCount = 0;
    qint64 m_lastTimeMs = 0;
};

#endif // MAINWINDOW_H
