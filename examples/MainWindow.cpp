#include "MainWindow.h"
#include "QtShaderCanvas.h"
#include <CodeEditor/CodeEditor.h>
#include <CodeEditor/EditorTheme.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
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
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QDir>
#include <QScrollArea>
#include <QMessageBox>
#include <QActionGroup>
#include <QTextStream>
#include <QCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    setupMenus();
    applyAesthetics();
    
    // Setup Drag and Drop
    setAcceptDrops(true);
    
    // Stats updates
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout, this, &MainWindow::updateStats);
    m_statsTimer->start(100); // 100ms update rate
    
    // Debounce compiler timer
    m_compileTimer = new QTimer(this);
    m_compileTimer->setSingleShot(true);
    m_compileTimer->setInterval(300); // 300ms debounce
    connect(m_compileTimer, &QTimer::timeout, this, &MainWindow::triggerCompile);
    
    m_lastTimeMs = QDateTime::currentMSecsSinceEpoch();

    // Load default shader into editor initially
    loadShaderIntoEditor(":/shaders/default.frag");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // Create the main horizontal splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setHandleWidth(6);
    mainSplitter->setObjectName("mainSplitter");
    
    // ==========================================
    // LEFT COLUMN: Editor (top) + Errors/Log (bottom)
    // ==========================================
    QSplitter *leftSplitter = new QSplitter(Qt::Vertical, this);
    leftSplitter->setHandleWidth(6);
    leftSplitter->setObjectName("leftSplitter");
    
    // Editor Container
    QWidget *editorContainer = new QWidget(this);
    QVBoxLayout *editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(4);
    
    QLabel *editorLabel = new QLabel(" 📝 GLSL EDITOR", this);
    editorLabel->setObjectName("panelHeader");
    
    m_editor = new CodeEditor(this);
    m_editor->setLineNumbersVisible(true);
    m_editor->setMiniMapVisible(true);
    m_editor->setFoldingEnabled(true);
    m_editor->setAutoCompleteEnabled(true);
    m_editor->setBracketPairGuidesEnabled(true);
    m_editor->setTabWidth(4);
    m_editor->setInsertSpacesOnTab(true);
    m_editor->setTheme(QEditorTheme::cursorDarkTheme()); // Sleek dark theme
    
    connect(m_editor, &CodeEditor::textChanged, this, &MainWindow::onEditorTextChanged);
    
    editorLayout->addWidget(editorLabel);
    editorLayout->addWidget(m_editor);
    
    // Errors/Log Container
    QFrame *logContainer = new QFrame(this);
    logContainer->setObjectName("panelContainer");
    QVBoxLayout *logLayout = new QVBoxLayout(logContainer);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->setSpacing(0);
    
    QLabel *logLabel = new QLabel(" 🖥️ ERRORS / LOG", this);
    logLabel->setObjectName("panelHeader");
    
    m_errorConsole = new QTextEdit(this);
    m_errorConsole->setObjectName("errorConsole");
    m_errorConsole->setReadOnly(true);
    m_errorConsole->setPlaceholderText("No compiler warnings or errors.");
    
    logLayout->addWidget(logLabel);
    logLayout->addWidget(m_errorConsole);
    
    leftSplitter->addWidget(editorContainer);
    leftSplitter->addWidget(logContainer);
    leftSplitter->setSizes(QList<int>() << 550 << 150);
    
    // ==========================================
    // RIGHT COLUMN: Preview (top) + Uniforms (bottom)
    // ==========================================
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->setHandleWidth(6);
    rightSplitter->setObjectName("rightSplitter");
    
    // Preview Container
    QFrame *previewContainer = new QFrame(this);
    previewContainer->setObjectName("panelContainer");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);
    
    QLabel *previewLabel = new QLabel(" 🎬 LIVE PREVIEW", this);
    previewLabel->setObjectName("panelHeader");
    
    m_canvas = new QtShaderCanvas(this);
    
    // Setup controls layout
    QWidget *controlsContainer = new QWidget(this);
    controlsContainer->setObjectName("controlsContainer");
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsContainer);
    controlsLayout->setContentsMargins(8, 8, 8, 8);
    controlsLayout->setSpacing(8);
    
    m_playButton = new QPushButton("▶ Play", this);
    m_pauseButton = new QPushButton("▮▮ Pause", this);
    m_stopButton = new QPushButton("■ Stop", this);
    m_stepButton = new QPushButton("⏵▮ Step", this);
    
    controlsLayout->addWidget(m_playButton);
    controlsLayout->addWidget(m_pauseButton);
    controlsLayout->addWidget(m_stopButton);
    controlsLayout->addWidget(m_stepButton);
    controlsLayout->addStretch(1);
    
    connect(m_playButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::play);
    connect(m_pauseButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::pause);
    connect(m_stopButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::stop);
    connect(m_stepButton, &QPushButton::clicked, m_canvas, &QtShaderCanvas::step);
    
    previewLayout->addWidget(previewLabel);
    previewLayout->addWidget(m_canvas, 1);
    previewLayout->addWidget(controlsContainer);
    
    // Uniform Inspector Container
    QFrame *inspectorContainer = new QFrame(this);
    inspectorContainer->setObjectName("panelContainer");
    QVBoxLayout *inspectorLayout = new QVBoxLayout(inspectorContainer);
    inspectorLayout->setContentsMargins(0, 0, 0, 0);
    inspectorLayout->setSpacing(0);
    
    QLabel *inspectorLabel = new QLabel(" 🔍 UNIFORM INSPECTOR", this);
    inspectorLabel->setObjectName("panelHeader");
    inspectorLayout->addWidget(inspectorLabel);
    
    // Scroll area for variables
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setObjectName("inspectorScroll");
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget *scrollContent = new QWidget(this);
    scrollContent->setObjectName("scrollContent");
    QFormLayout *formLayout = new QFormLayout(scrollContent);
    formLayout->setContentsMargins(12, 12, 12, 12);
    formLayout->setSpacing(10);
    
    // Read-only labels for uniform values
    m_inspectTime = new QLabel("0.00 s", this);
    m_inspectTime->setObjectName("inspectorValue");
    m_inspectFrame = new QLabel("0", this);
    m_inspectFrame->setObjectName("inspectorValue");
    m_inspectResolution = new QLabel("0 x 0", this);
    m_inspectResolution->setObjectName("inspectorValue");
    m_inspectMouse = new QLabel("(0.0, 0.0)", this);
    m_inspectMouse->setObjectName("inspectorValue");
    m_inspectDate = new QLabel("----/--/-- --:--:--", this);
    m_inspectDate->setObjectName("inspectorValue");
    
    // Channel 0 texture picker
    m_channel0Selector = new QComboBox(this);
    m_channel0Selector->addItem("None (Black)");
    m_channel0Selector->addItem("Green Lush Image");
    m_channel0Selector->addItem("Browse Custom Image...");
    connect(m_channel0Selector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onChannel0Changed);
    
    // Add to form
    formLayout->addRow("iTime (float):", m_inspectTime);
    formLayout->addRow("iFrame (int):", m_inspectFrame);
    formLayout->addRow("iResolution (vec3):", m_inspectResolution);
    formLayout->addRow("iMouse (vec4):", m_inspectMouse);
    formLayout->addRow("iDate (vec4):", m_inspectDate);
    formLayout->addRow("iChannel0 (sampler2D):", m_channel0Selector);
    
    // Add separator
    QFrame *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background-color: #21262d; max-height: 1px;");
    formLayout->addRow(sep);
    
    // Configuration inline controls
    m_hotReloadToggle = new QCheckBox("Auto Compile on Type", this);
    m_hotReloadToggle->setChecked(true);
    formLayout->addRow("Auto Compile:", m_hotReloadToggle);
    
    m_fpsSliderLabel = new QLabel("60 FPS", this);
    m_fpsSliderLabel->setObjectName("inspectorValue");
    m_fpsSlider = new QSlider(Qt::Horizontal, this);
    m_fpsSlider->setRange(0, 144);
    m_fpsSlider->setValue(60);
    m_fpsSlider->setSingleStep(10);
    connect(m_fpsSlider, &QSlider::valueChanged, this, &MainWindow::onFpsLimitChanged);
    
    QHBoxLayout *fpsLayout = new QHBoxLayout();
    fpsLayout->setContentsMargins(0, 0, 0, 0);
    fpsLayout->addWidget(m_fpsSlider, 1);
    fpsLayout->addWidget(m_fpsSliderLabel);
    formLayout->addRow("FPS Limit:", fpsLayout);
    
    scrollArea->setWidget(scrollContent);
    inspectorLayout->addWidget(scrollArea);
    
    rightSplitter->addWidget(previewContainer);
    rightSplitter->addWidget(inspectorContainer);
    rightSplitter->setSizes(QList<int>() << 450 << 250);
    
    // Add both columns to main horizontal splitter
    mainSplitter->addWidget(leftSplitter);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setSizes(QList<int>() << 600 << 600);
    
    mainLayout->addWidget(mainSplitter);
    
    // Connections from Canvas
    connect(m_canvas, &QtShaderCanvas::playStateChanged, this, &MainWindow::onPlayStateChanged);
    connect(m_canvas, &QtShaderCanvas::shaderLoaded, this, &MainWindow::onShaderLoaded);
    connect(m_canvas, &QtShaderCanvas::shaderError, this, &MainWindow::onShaderError);
}

void MainWindow::setupMenus() {
    QMenuBar *mBar = menuBar();
    
    // --- File Menu ---
    QMenu *fileMenu = mBar->addMenu("&File");
    
    QAction *newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onFileNew);
    
    QAction *openAction = fileMenu->addAction("&Open File...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onFileOpen);
    
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onFileSave);
    
    QAction *saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onFileSaveAs);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("&Exit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // --- Edit Menu ---
    QMenu *editMenu = mBar->addMenu("&Edit");
    
    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, m_editor, &CodeEditor::undo);
    
    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, m_editor, &CodeEditor::redo);
    
    editMenu->addSeparator();
    
    QAction *cutAction = editMenu->addAction("Cu&t");
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, m_editor, &CodeEditor::cut);
    
    QAction *copyAction = editMenu->addAction("&Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, m_editor, &CodeEditor::copy);
    
    QAction *pasteAction = editMenu->addAction("&Paste");
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, m_editor, &CodeEditor::paste);
    
    editMenu->addSeparator();
    
    QAction *selectAllAction = editMenu->addAction("Select &All");
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, m_editor, &CodeEditor::selectAll);
    
    // --- View Menu ---
    QMenu *viewMenu = mBar->addMenu("&View");
    
    QAction *toggleMinimap = viewMenu->addAction("Show &Minimap");
    toggleMinimap->setCheckable(true);
    toggleMinimap->setChecked(true);
    connect(toggleMinimap, &QAction::toggled, m_editor, &CodeEditor::setMiniMapVisible);
    
    QAction *toggleLineNumbers = viewMenu->addAction("Show &Line Numbers");
    toggleLineNumbers->setCheckable(true);
    toggleLineNumbers->setChecked(true);
    connect(toggleLineNumbers, &QAction::toggled, m_editor, &CodeEditor::setLineNumbersVisible);
    
    QAction *toggleFolding = viewMenu->addAction("Enable Code &Folding");
    toggleFolding->setCheckable(true);
    toggleFolding->setChecked(true);
    connect(toggleFolding, &QAction::toggled, m_editor, &CodeEditor::setFoldingEnabled);
    
    viewMenu->addSeparator();
    
    QAction *zoomIn = viewMenu->addAction("Zoom &In");
    zoomIn->setShortcut(QKeySequence::ZoomIn);
    connect(zoomIn, &QAction::triggered, this, [this]() {
        QFont f = m_editor->editorFont();
        f.setPointSize(f.pointSize() + 1);
        m_editor->setEditorFont(f);
    });
    
    QAction *zoomOut = viewMenu->addAction("Zoom &Out");
    zoomOut->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOut, &QAction::triggered, this, [this]() {
        QFont f = m_editor->editorFont();
        f.setPointSize(qMax(6, f.pointSize() - 1));
        m_editor->setEditorFont(f);
    });
    
    // --- Examples Menu ---
    QMenu *examplesMenu = mBar->addMenu("&Examples");
    
    QAction *defaultEx = examplesMenu->addAction("Default Ripple (Built-in)");
    connect(defaultEx, &QAction::triggered, this, [this]() {
        loadShaderIntoEditor(":/shaders/default.frag");
    });
    
    QAction *starNestEx = examplesMenu->addAction("Star Nest Galaxy (Built-in)");
    connect(starNestEx, &QAction::triggered, this, [this]() {
        loadShaderIntoEditor(":/shaders/star_nest.frag");
    });
    
    examplesMenu->addSeparator();
    
    // Search the GLSL-Shaders folder dynamically
    QString shaderDirPath = "GLSL-Shaders";
    if (!QDir(shaderDirPath).exists()) {
        shaderDirPath = "../GLSL-Shaders";
    }
    if (!QDir(shaderDirPath).exists()) {
        shaderDirPath = "/home/nord/code_base/QtShaderCanvas/GLSL-Shaders";
    }
    
    QDir dir(shaderDirPath);
    if (dir.exists()) {
        QStringList shaders = dir.entryList(QStringList() << "*.glsl" << "*.frag", QDir::Files | QDir::Readable, QDir::Name);
        if (!shaders.isEmpty()) {
            QMenu *folderMenu = examplesMenu->addMenu("📚 Shaders Folder");
            for (const QString &shader : shaders) {
                QAction *act = folderMenu->addAction(shader);
                QString fullPath = dir.absoluteFilePath(shader);
                connect(act, &QAction::triggered, this, [this, fullPath]() {
                    loadShaderIntoEditor(fullPath);
                });
            }
        }
    }
    
    // --- Settings Menu ---
    QMenu *settingsMenu = mBar->addMenu("&Settings");
    
    QAction *autoCompileAct = settingsMenu->addAction("Auto Compile on Type");
    autoCompileAct->setCheckable(true);
    autoCompileAct->setChecked(true);
    connect(autoCompileAct, &QAction::toggled, this, [this](bool checked) {
        m_hotReloadToggle->setChecked(checked);
    });
    connect(m_hotReloadToggle, &QCheckBox::toggled, autoCompileAct, &QAction::setChecked);
    
    settingsMenu->addSeparator();
    
    // FPS Limits Submenu
    QMenu *fpsMenu = settingsMenu->addMenu("⏱️ FPS Limit");
    QActionGroup *fpsGroup = new QActionGroup(this);
    
    auto addFpsAction = [this, fpsMenu, fpsGroup](const QString &text, int limit) {
        QAction *act = fpsMenu->addAction(text);
        act->setCheckable(true);
        act->setChecked(m_canvas->fpsLimit() == limit);
        fpsGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, limit]() {
            m_fpsSlider->setValue(limit);
        });
        return act;
    };
    
    addFpsAction("VSync (Unlimited)", 0);
    addFpsAction("30 FPS", 30);
    addFpsAction("60 FPS", 60);
    addFpsAction("120 FPS", 120);
    addFpsAction("144 FPS", 144);
}

void MainWindow::applyAesthetics() {
    // Beautiful, dark premium stylesheet themed after Github Dark / VSCode
    QString qss = R"(
        QMainWindow {
            background-color: #0d1117;
        }

        QSplitter::handle {
            background-color: #21262d;
        }

        QSplitter::handle:hover {
            background-color: #58a6ff;
        }

        QLabel {
            color: #c9d1d9;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 12px;
        }

        QLabel#panelHeader {
            color: #58a6ff;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 11px;
            font-weight: bold;
            text-transform: uppercase;
            letter-spacing: 1.2px;
            padding: 8px 12px;
            background-color: #161b22;
            border-bottom: 1px solid #21262d;
        }

        QLabel#inspectorValue {
            color: #58a6ff;
            font-family: 'Consolas', 'Courier New', monospace;
            font-weight: bold;
            font-size: 12px;
        }

        QFrame#panelContainer {
            background-color: #0d1117;
            border: 1px solid #30363d;
        }

        QWidget#controlsContainer {
            background-color: #161b22;
            border-top: 1px solid #21262d;
        }

        QTextEdit#errorConsole {
            background-color: #090d13;
            border: none;
            color: #f85149;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
            padding: 10px;
        }

        QPushButton {
            background-color: #21262d;
            border: 1px solid #30363d;
            border-radius: 4px;
            padding: 6px 12px;
            color: #c9d1d9;
            font-weight: bold;
            font-size: 12px;
        }

        QPushButton:hover {
            background-color: #30363d;
            border-color: #8b949e;
        }

        QPushButton:pressed {
            background-color: #58a6ff;
            color: #0d1117;
        }
        
        QPushButton:disabled {
            background-color: #0d1117;
            color: #484f58;
            border-color: #21262d;
        }

        QComboBox {
            background-color: #0d1117;
            border: 1px solid #30363d;
            border-radius: 4px;
            padding: 4px 8px;
            color: #c9d1d9;
            min-height: 25px;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left-width: 1px;
            border-left-color: #30363d;
            border-left-style: solid;
        }

        QComboBox QAbstractItemView {
            background-color: #0d1117;
            color: #c9d1d9;
            selection-background-color: #58a6ff;
            selection-color: #0d1117;
            border: 1px solid #30363d;
        }

        QCheckBox {
            color: #c9d1d9;
            spacing: 8px;
        }

        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #30363d;
            border-radius: 3px;
            background-color: #0d1117;
        }

        QCheckBox::indicator:checked {
            background-color: #58a6ff;
            border-color: #58a6ff;
        }

        QSlider::groove:horizontal {
            border: 1px solid #21262d;
            height: 6px;
            background: #161b22;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background: #58a6ff;
            border: 1px solid #58a6ff;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }

        QSlider::handle:horizontal:hover {
            background: #ffffff;
            border-color: #ffffff;
        }

        QMenuBar {
            background-color: #161b22;
            border-bottom: 1px solid #21262d;
            color: #c9d1d9;
        }

        QMenuBar::item {
            background-color: transparent;
            padding: 6px 12px;
        }

        QMenuBar::item:selected {
            background-color: #21262d;
            border-radius: 4px;
        }

        QMenu {
            background-color: #161b22;
            border: 1px solid #30363d;
            color: #c9d1d9;
        }

        QMenu::item {
            padding: 6px 20px;
        }

        QMenu::item:selected {
            background-color: #58a6ff;
            color: #0d1117;
        }
        
        QScrollArea {
            background-color: transparent;
        }
        
        QWidget#scrollContent {
            background-color: transparent;
        }
    )";
    
    setStyleSheet(qss);
    setWindowTitle("⚡ ShaderCanvas Playground");
    resize(1280, 800);
}

void MainWindow::onEditorTextChanged() {
    if (m_hotReloadToggle->isChecked()) {
        m_compileTimer->start(); // restart debounce timer
    }
}

void MainWindow::triggerCompile() {
    QString code = m_editor->text();
    m_canvas->loadShaderCode(code);
}

void MainWindow::onShaderLoaded(const QString &path) {
    m_errorConsole->clear();
    m_errorConsole->setTextColor(QColor("#4ade80")); // success green
    m_errorConsole->append(QString("[%1] Compilation Successful.").arg(QTime::currentTime().toString("hh:mm:ss")));
    if (!path.isEmpty() && !path.startsWith(":/")) {
        m_errorConsole->append(QString("Loaded file: %1").arg(QFileInfo(path).fileName()));
    }
    m_errorConsole->setTextColor(QColor("#c9d1d9"));
}

void MainWindow::onShaderError(const QString &error) {
    m_errorConsole->clear();
    m_errorConsole->setTextColor(QColor("#f85149")); // error red
    m_errorConsole->append(error);
    m_errorConsole->setTextColor(QColor("#c9d1d9"));
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

void MainWindow::onFpsLimitChanged(int value) {
    m_canvas->setFpsLimit(value);
    updateFpsLabel(value);
}

void MainWindow::updateFpsLabel(int fps) {
    if (fps <= 0) {
        m_fpsSliderLabel->setText("Unlimited");
    } else {
        m_fpsSliderLabel->setText(QString("%1 FPS").arg(fps));
    }
}

void MainWindow::updateStats() {
    if (!m_canvas) return;
    
    m_inspectTime->setText(QString("%1 s").arg(m_canvas->time(), 0, 'f', 2));
    m_inspectFrame->setText(QString::number(m_canvas->frame()));
    
    float ratio = devicePixelRatioF();
    m_inspectResolution->setText(QString("%1 x %2").arg(m_canvas->width() * ratio).arg(m_canvas->height() * ratio));
    
    QPoint localPos = m_canvas->mapFromGlobal(QCursor::pos());
    if (m_canvas->rect().contains(localPos)) {
        m_inspectMouse->setText(QString("(%1, %2)").arg(localPos.x() * ratio).arg((m_canvas->height() - localPos.y()) * ratio));
    }
    
    m_inspectDate->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

void MainWindow::onFileNew() {
    m_currentFilePath.clear();
    m_editor->blockSignals(true);
    m_editor->setText(
        "void mainImage(out vec4 fragColor, in vec2 fragCoord) {\n"
        "    // Normalized pixel coordinates (from 0 to 1)\n"
        "    vec2 uv = fragCoord / iResolution.xy;\n"
        "\n"
        "    // Time-varying pixel color\n"
        "    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0.0, 2.0, 4.0));\n"
        "\n"
        "    // Output to screen\n"
        "    fragColor = vec4(col, 1.0);\n"
        "}\n"
    );
    m_editor->blockSignals(false);
    triggerCompile();
}

void MainWindow::onFileOpen() {
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("Open GLSL Fragment Shader"), "",
        tr("GLSL Shaders (*.frag *.glsl *.txt);;All Files (*)"));
        
    if (!filePath.isEmpty()) {
        loadShaderIntoEditor(filePath);
    }
}

void MainWindow::onFileSave() {
    if (m_currentFilePath.isEmpty() || m_currentFilePath.startsWith(":/")) {
        onFileSaveAs();
    } else {
        QFile file(m_currentFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_editor->text();
            file.close();
            m_errorConsole->setTextColor(QColor("#4ade80"));
            m_errorConsole->append(QString("[%1] Saved: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(QFileInfo(m_currentFilePath).fileName()));
            m_errorConsole->setTextColor(QColor("#c9d1d9"));
        } else {
            QMessageBox::warning(this, "Save Error", "Could not save file to " + m_currentFilePath);
        }
    }
}

void MainWindow::onFileSaveAs() {
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("Save GLSL Fragment Shader"), "",
        tr("GLSL Shaders (*.frag *.glsl *.txt);;All Files (*)"));
        
    if (!filePath.isEmpty()) {
        m_currentFilePath = filePath;
        onFileSave();
    }
}

void MainWindow::loadShaderIntoEditor(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Open Error", "Could not open file: " + filePath);
        return;
    }
    
    QTextStream in(&file);
    QString code = in.readAll();
    file.close();
    
    m_editor->blockSignals(true);
    m_editor->setText(code);
    m_editor->blockSignals(false);
    
    m_currentFilePath = filePath;
    triggerCompile();
}

void MainWindow::onChannel0Changed(int index) {
    if (index == 0) {
        m_canvas->setBackgroundImage("");
    } else if (index == 1) {
        QString bgPath = "06. Green Lush.jpg";
        if (!QFile::exists(bgPath)) bgPath = "../06. Green Lush.jpg";
        if (!QFile::exists(bgPath)) bgPath = "/home/nord/code_base/QtShaderCanvas/06. Green Lush.jpg";
        m_canvas->setBackgroundImage(bgPath);
    } else if (index == 2) {
        QString filePath = QFileDialog::getOpenFileName(this,
            tr("Select Channel 0 Texture"), "",
            tr("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)"));
        if (!filePath.isEmpty()) {
            m_canvas->setBackgroundImage(filePath);
        } else {
            // Revert selection to none if cancelled
            m_channel0Selector->blockSignals(true);
            m_channel0Selector->setCurrentIndex(0);
            m_channel0Selector->blockSignals(false);
            m_canvas->setBackgroundImage("");
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
        loadShaderIntoEditor(filePath);
    }
}
