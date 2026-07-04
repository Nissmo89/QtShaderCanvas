#include "MainWindow.h"
#include "QtShaderCanvas.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QTextEdit>
#include <QSlider>
#include <QFileDialog>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>
#include <QFrame>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    applyAesthetics();
    
    // Setup Drag and Drop
    setAcceptDrops(true);
    
    // Stats updates
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout, this, &MainWindow::updateStats);
    m_statsTimer->start(500); // update every 500ms
    m_lastTimeMs = QDateTime::currentMSecsSinceEpoch();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 1. Create the Canvas
    m_canvas = new QtShaderCanvas(this);
    
    // Set background image
    QString bgPath = "06. Green Lush.jpg";
    if (!QFile::exists(bgPath)) {
        bgPath = "../06. Green Lush.jpg";
    }
    if (!QFile::exists(bgPath)) {
        bgPath = "/home/nord/code_base/QtShaderCanvas/06. Green Lush.jpg";
    }
    m_canvas->setBackgroundImage(bgPath);

    mainLayout->addWidget(m_canvas, 1); // stretches to fill
    
    // 2. Create the Sidebar Panel
    QFrame *sidebar = new QFrame(this);
    sidebar->setObjectName("sidebar");
    
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 20, 15, 15);
    sidebarLayout->setSpacing(10);
    
    // Logo & Titles
    QLabel *titleLabel = new QLabel("QT SHADER CANVAS", this);
    titleLabel->setObjectName("titleLabel");
    
    QLabel *subtitleLabel = new QLabel("ShaderToy Widgets Viewport", this);
    subtitleLabel->setObjectName("subtitleLabel");
    
    sidebarLayout->addWidget(titleLabel);
    sidebarLayout->addWidget(subtitleLabel);
    sidebarLayout->addSpacing(15);
    
    // --- SECTION: PRESETS ---
    QLabel *presetsHeader = new QLabel("Presets", this);
    presetsHeader->setObjectName("headerLabel");
    sidebarLayout->addWidget(presetsHeader);
    
    m_shaderSelector = new QComboBox(this);
    m_shaderSelector->addItem("Default Ripple (Boilerplate)");
    m_shaderSelector->addItem("Star Nest Galaxy (Kali)");
    connect(m_shaderSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onShaderSelected);
    sidebarLayout->addWidget(m_shaderSelector);
    
    m_openButton = new QPushButton("📂 Open GLSL Fragment File", this);
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::onLoadFileClicked);
    sidebarLayout->addWidget(m_openButton);
    
    sidebarLayout->addSpacing(10);
    
    // --- SECTION: CONTROLS ---
    QLabel *controlsHeader = new QLabel("Controls", this);
    controlsHeader->setObjectName("headerLabel");
    sidebarLayout->addWidget(controlsHeader);
    
    QGridLayout *controlsGrid = new QGridLayout();
    controlsGrid->setSpacing(8);
    
    m_playButton = new QPushButton("▶ Play", this);
    m_pauseButton = new QPushButton("▮▮ Pause", this);
    m_stopButton = new QPushButton("■ Stop", this);
    m_stepButton = new QPushButton("⏵▮ Step", this);
    
    controlsGrid->addWidget(m_playButton, 0, 0);
    controlsGrid->addWidget(m_pauseButton, 0, 1);
    controlsGrid->addWidget(m_stopButton, 1, 0);
    controlsGrid->addWidget(m_stepButton, 1, 1);
    
    sidebarLayout->addLayout(controlsGrid);
    
    // Connect controls
    connect(m_playButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::play);
    connect(m_pauseButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::pause);
    connect(m_stopButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::stop);
    connect(m_stepButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::step);
    
    sidebarLayout->addSpacing(10);
    
    // --- SECTION: CONFIG ---
    QLabel *configHeader = new QLabel("Configuration", this);
    configHeader->setObjectName("headerLabel");
    sidebarLayout->addWidget(configHeader);
    
    m_hotReloadToggle = new QCheckBox("Auto Hot Reload", this);
    m_hotReloadToggle->setChecked(true);
    connect(m_hotReloadToggle, &QCheckBox::toggled, m_canvas, &QtShaderCanvas::setHotReloadEnabled);
    sidebarLayout->addWidget(m_hotReloadToggle);
    
    // FPS Slider
    QHBoxLayout *fpsLayout = new QHBoxLayout();
    m_fpsSliderLabel = new QLabel("FPS Limit: 60", this);
    fpsLayout->addWidget(m_fpsSliderLabel);
    
    m_fpsSlider = new QSlider(Qt::Horizontal, this);
    m_fpsSlider->setRange(0, 144);
    m_fpsSlider->setValue(60);
    m_fpsSlider->setSingleStep(10);
    connect(m_fpsSlider, &QSlider::valueChanged, this, &MainWindow::onFpsLimitChanged);
    
    sidebarLayout->addLayout(fpsLayout);
    sidebarLayout->addWidget(m_fpsSlider);
    
    sidebarLayout->addSpacing(10);
    
    // --- SECTION: STATS ---
    QLabel *statsHeader = new QLabel("Statistics", this);
    statsHeader->setObjectName("headerLabel");
    sidebarLayout->addWidget(statsHeader);
    
    m_timeLabel = new QLabel("Time: 0.00s", this);
    m_frameLabel = new QLabel("Frame: 0", this);
    m_fpsCounterLabel = new QLabel("FPS: --", this);
    
    sidebarLayout->addWidget(m_timeLabel);
    sidebarLayout->addWidget(m_frameLabel);
    sidebarLayout->addWidget(m_fpsCounterLabel);
    
    sidebarLayout->addSpacing(10);
    
    // Status label at the bottom of controls
    m_statusLabel = new QLabel("Loaded: Built-in default", this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #45a29e; font-style: italic; font-size: 11px;");
    sidebarLayout->addWidget(m_statusLabel);
    
    sidebarLayout->addStretch(1);
    
    // --- SECTION: COMPILER LOG ---
    m_errorConsole = new QTextEdit(this);
    m_errorConsole->setObjectName("errorConsole");
    m_errorConsole->setReadOnly(true);
    m_errorConsole->setVisible(false); // only visible when there's an error
    sidebarLayout->addWidget(m_errorConsole);
    
    mainLayout->addWidget(sidebar);
    
    // Canvas connections
    connect(m_canvas, &QtShaderCanvas::playStateChanged, this, &MainWindow::onPlayStateChanged);
    connect(m_canvas, &QtShaderCanvas::shaderLoaded, this, &MainWindow::onShaderLoaded);
    connect(m_canvas, &QtShaderCanvas::shaderError, this, &MainWindow::onShaderError);
    connect(m_canvas, &QtShaderCanvas::fpsLimitChanged, this, &MainWindow::onFpsLimitChanged);
    
    // Trigger initial load of default
    onShaderSelected(0);
}

void MainWindow::applyAesthetics() {
    // Styling sheet matching dark premium aesthetics
    QString qss = R"(
        QMainWindow {
            background-color: #0b0c10;
        }

        QFrame#sidebar {
            background-color: #151b24;
            border-left: 2px solid #1f2833;
            min-width: 290px;
            max-width: 290px;
        }

        QLabel {
            color: #c5c6c7;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 12px;
        }

        QLabel#titleLabel {
            color: #66fcf1;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 1.5px;
        }

        QLabel#subtitleLabel {
            color: #45a29e;
            font-size: 11px;
            font-weight: 500;
        }

        QLabel#headerLabel {
            color: #66fcf1;
            font-size: 11px;
            font-weight: bold;
            text-transform: uppercase;
            margin-top: 10px;
            margin-bottom: 2px;
            letter-spacing: 0.5px;
        }

        QComboBox {
            background-color: #0b0c10;
            border: 1px solid #45a29e;
            border-radius: 4px;
            padding: 5px 8px;
            color: #c5c6c7;
            min-height: 25px;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left-width: 1px;
            border-left-color: #45a29e;
            border-left-style: solid;
            border-top-right-radius: 3px;
            border-bottom-right-radius: 3px;
        }

        QComboBox QAbstractItemView {
            background-color: #0b0c10;
            color: #c5c6c7;
            selection-background-color: #45a29e;
            selection-color: #0b0c10;
            border: 1px solid #45a29e;
        }

        QPushButton {
            background-color: #0b0c10;
            border: 1px solid #45a29e;
            border-radius: 4px;
            padding: 8px 12px;
            color: #c5c6c7;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #45a29e;
            color: #0b0c10;
            border: 1px solid #66fcf1;
        }

        QPushButton:pressed {
            background-color: #66fcf1;
            color: #0b0c10;
        }

        QCheckBox {
            color: #c5c6c7;
            spacing: 8px;
        }

        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #45a29e;
            border-radius: 3px;
            background-color: #0b0c10;
        }

        QCheckBox::indicator:checked {
            background-color: #66fcf1;
            border: 1px solid #66fcf1;
        }

        QSlider::groove:horizontal {
            border: 1px solid #1f2833;
            height: 6px;
            background: #0b0c10;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background: #66fcf1;
            border: 1px solid #45a29e;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }

        QSlider::handle:horizontal:hover {
            background: #ffffff;
            border: 1px solid #66fcf1;
        }

        QTextEdit#errorConsole {
            background-color: #120507;
            border: 1px solid #ff4c60;
            border-radius: 4px;
            color: #ff4c60;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 10px;
            min-height: 120px;
            max-height: 180px;
        }
    )";
    
    setStyleSheet(qss);
    setWindowTitle("QtShaderCanvas - ShaderToy Compatibility Player");
    resize(1024, 600);
}

void MainWindow::onShaderSelected(int index) {
    QString path;
    if (index == 0) {
        path = ":/shaders/default.frag";
    } else if (index == 1) {
        path = ":/shaders/star_nest.frag";
    }
    
    if (!path.isEmpty()) {
        m_errorConsole->clear();
        m_errorConsole->setVisible(false);
        m_canvas->loadShader(path);
        m_statusLabel->setText("Loaded Built-in: " + (index == 0 ? QString("default.frag") : QString("star_nest.frag")));
    }
}

void MainWindow::onLoadFileClicked() {
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Open GLSL Fragment Shader"), "",
        tr("GLSL Shaders (*.frag *.glsl *.txt);;All Files (*)"));
        
    if (!filePath.isEmpty()) {
        if (m_canvas->loadShader(filePath)) {
            m_shaderSelector->blockSignals(true);
            m_shaderSelector->setCurrentIndex(-1);
            m_shaderSelector->blockSignals(false);
            
            m_statusLabel->setText(QFileInfo(filePath).fileName());
            m_errorConsole->clear();
            m_errorConsole->setVisible(false);
        }
    }
}

void MainWindow::onPlayStateChanged(bool playing) {
    if (playing) {
        m_playButton->setEnabled(false);
        m_pauseButton->setEnabled(true);
    } else {
        m_playButton->setEnabled(true);
        m_pauseButton->setEnabled(false);
    }
}

void MainWindow::onShaderLoaded(const QString &path) {
    m_statusLabel->setText("Watched: " + QFileInfo(path).fileName());
    m_errorConsole->clear();
    m_errorConsole->setVisible(false);
}

void MainWindow::onShaderError(const QString &error) {
    m_errorConsole->setText(error);
    m_errorConsole->setVisible(true);
}

void MainWindow::onFpsLimitChanged(int value) {
    m_canvas->setFpsLimit(value);
    updateFpsLabel(value);
}

void MainWindow::updateFpsLabel(int fps) {
    if (fps <= 0) {
        m_fpsSliderLabel->setText("FPS Limit: Unlimited (VSync)");
    } else {
        m_fpsSliderLabel->setText(QString("FPS Limit: %1").arg(fps));
    }
}

void MainWindow::updateStats() {
    if (!m_canvas) return;
    
    m_timeLabel->setText(QString("Time: %1s").arg(m_canvas->time(), 0, 'f', 2));
    m_frameLabel->setText(QString("Frame: %1").arg(m_canvas->frame()));
    
    // FPS counter calculation
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 delta = now - m_lastTimeMs;
    if (delta >= 500) {
        int currentFrame = m_canvas->frame();
        int frameDelta = currentFrame - m_lastFrameCount;
        float fps = (frameDelta * 1000.0f) / delta;
        
        m_lastFrameCount = currentFrame;
        m_lastTimeMs = now;
        
        if (m_canvas->isPlaying()) {
            m_fpsCounterLabel->setText(QString("Measured FPS: %1").arg(fps, 0, 'f', 1));
        } else {
            m_fpsCounterLabel->setText("Measured FPS: Paused");
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1) {
        QString file = event->mimeData()->urls().first().toLocalFile();
        if (file.endsWith(".glsl") || file.endsWith(".frag") || file.endsWith(".txt")) {
            event->acceptProposedAction();
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1) {
        QString filePath = event->mimeData()->urls().first().toLocalFile();
        if (m_canvas->loadShader(filePath)) {
            m_shaderSelector->blockSignals(true);
            m_shaderSelector->setCurrentIndex(-1);
            m_shaderSelector->blockSignals(false);
            
            m_statusLabel->setText(QFileInfo(filePath).fileName());
            m_errorConsole->clear();
            m_errorConsole->setVisible(false);
        }
    }
}
