#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./database/dbmanager.h"
#include "./ui/tabledialog.h"
#include "gncdb.h"
#include "vacuum.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QHeaderView>
#include <QToolBar>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QVector>
#include <QStringList>
#include <QFileDialog>
#include <QIcon>
#include <QCursor>
#include <QSplitter>
#include <QApplication>
#include <QTextBlock>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QToolButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSizePolicy>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>  // 添加QPainterPath头文件
#include "rc2msg.h"
// 新添加的头文件
#include <QSettings>
#include <QCloseEvent>
#include <QCoreApplication>

// 定义一个结构体来存储表名
struct TableInfo
{
    QString tableName;
};

// 定义一个全局变量来存储表名列表
QVector<TableInfo> tableList;

// C++版本的callback函数
int cppCallback(void *data, int argc, char ** /*azColName*/, char **argv)
{
    QVector<TableInfo> *tables = static_cast<QVector<TableInfo> *>(data);

    // 添加调试信息
    qDebug() << "Callback被调用，列数:" << argc;

    // 假设第一列是表名
    if (argc > 0 && argv[0])
    {
        TableInfo table;
        table.tableName = QString::fromUtf8(argv[0]);
        tables->append(table);
        qDebug() << "添加表名:" << table.tableName;
    }

    return 0;
}

// 修改callback函数以适应GNCDB_select
int selectCallback(void *data, int argc, char ** /*azColName*/, char **argv)
{
    QVector<TableInfo> *tables = static_cast<QVector<TableInfo> *>(data);

    // 添加调试信息
    qDebug() << "SelectCallback被调用，列数:" << argc;

    // 假设第一列是表名
    if (argc > 0 && argv[0])
    {
        TableInfo table;
        table.tableName = QString::fromUtf8(argv[0]);
        tables->append(table);
        qDebug() << "添加表名:" << table.tableName;
    }

    return 0;
}

// 用于SQL查询结果的callback函数
int sqlResultCallback(void *data, int argc, char **azColName, char **argv)
{
    SQLResult *result = static_cast<SQLResult *>(data);

    // qDebug() << "sqlResultCallback被调用，列数:" << argc;

    // 如果是第一次调用，保存列名
    if (result->columnNames.isEmpty())
    {
        // qDebug() << "保存列名:";
        for (int i = 0; i < argc; i++)
        {
            QString colName = QString::fromUtf8(azColName[i]);
            result->columnNames.append(colName);
            // qDebug() << "  列" << i << ":" << colName;
        }
    }

    // 保存行数据
    QStringList row;
    // qDebug() << "保存行数据:";
    for (int i = 0; i < argc; i++)
    {
        QString value = argv[i] ? QString::fromUtf8(argv[i]) : "NULL";
        row.append(value);
        // qDebug() << "  列" << i << ":" << value;
    }
    result->rows.append(row);

    return 0;
}

// TreeItemDelegate implementation
TreeItemDelegate::TreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    databaseIcon = QIcon(":/icons/database_logo.png");
    tableIcon = QIcon(":/icons/table_logo.png");
    columnIcon = QIcon(":/icons/column_logo.png");
}

void TreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // 保存画笔状态
    painter->save();

    // 绘制选中状态背景
    if (opt.state & QStyle::State_Selected)
    {
        painter->fillRect(opt.rect, opt.palette.highlight());
    }

    // 计算图标和文本的位置
    QRect iconRect = opt.rect;
    iconRect.setWidth(16);                                           // 图标宽度
    iconRect.setHeight(16);                                          // 图标高度
    iconRect.moveTop(opt.rect.top() + (opt.rect.height() - 16) / 2); // 垂直居中

    QRect textRect = opt.rect;
    textRect.setLeft(opt.rect.left() + 20); // 文本左边留出空间给图标

    // 获取节点类型
    int nodeType = index.data(Qt::UserRole).toInt();

    // 根据节点类型选择图标
    QIcon icon;
    switch (nodeType)
    {
    case 0: // 数据库节点
        icon = databaseIcon;
        break;
    case 1: // 表节点
        icon = tableIcon;
        break;
    case 2: // 列节点
        icon = columnIcon;
        break;
    }

    if (!icon.isNull())
    {
        icon.paint(painter, iconRect);
    }

    // 绘制文本
    painter->setPen(opt.state & QStyle::State_Selected ? opt.palette.highlightedText().color() : opt.palette.text().color());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, index.data().toString());

    // 恢复画笔状态
    painter->restore();
}

QSize TreeItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), 20)); // 确保至少20像素高
    return size;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), dbManager(new DBManager()), db(nullptr), mainSplitter(nullptr), tableTree(nullptr), rightTabWidget(nullptr), dataTable(nullptr), sqlEditor(nullptr), databaseTab(nullptr), statusBar(nullptr), ddlEditor(nullptr), sqlResultDisplay(nullptr), sqlHighlighter(nullptr), dbNameLabel(nullptr), tableNameLabel(nullptr), dbPathLabel(nullptr), dbTitleLabel(nullptr)
{
    ui->setupUi(this);
    
    // 设置程序标题
    setWindowTitle("GNCDB - 数据库管理工具");
    
    // 初始化设置文件路径
    QDir appDir = QDir(QCoreApplication::applicationDirPath());
    settingsFilePath = appDir.filePath("settings.ini");
    
    // 加载上次的窗口设置
    loadWindowSettings();
    
    // 初始化界面
    initUI();
    
    // 初始化SQL高亮器
    if (sqlEditor && sqlEditor->document())
    {
        sqlHighlighter = new SqlHighlighter(sqlEditor->document());
        if (sqlHighlighter)
        {
            sqlHighlighter->setTheme("Light");
            sqlHighlighter->addCustomKeywords({"CUSTOM_FUNCTION", "SPECIAL_TABLE"});
        }
    }

    initConnections();
    

}

MainWindow::~MainWindow()
{
    // 确保在主线程中删除高亮器
    if (sqlHighlighter)
    {
        delete sqlHighlighter;
        sqlHighlighter = nullptr;
    }

    if (db)
    {
        GNCDB_close(&db);
        db = nullptr;
    }
    delete dbManager;
    delete ui;
}

void MainWindow::initUI()
{
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    // 创建菜单栏
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // 文件菜单
    fileMenu = menuBar->addMenu(tr("文件(&F)"));
    QAction *newDbAction = fileMenu->addAction(tr("新建数据库"), this, &MainWindow::onNewDatabase);
    newDbAction->setShortcut(QKeySequence("Ctrl+N"));
    QAction *openDbAction = fileMenu->addAction(tr("打开数据库"), this, &MainWindow::onConnectDB);
    openDbAction->setShortcut(QKeySequence("Ctrl+O"));
    QAction *disconnectDbAction = fileMenu->addAction(tr("断开连接"), this, &MainWindow::onDisconnectDB);
    disconnectDbAction->setShortcut(QKeySequence("Ctrl+D"));
    disconnectDbAction->setIcon(QIcon(":/icons/disconnect.png")); // 添加断开连接图标
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction(tr("退出"), this, &MainWindow::onExit);
    exitAction->setShortcut(QKeySequence("Alt+F4"));
    exitAction->setIcon(QIcon(":/icons/exit.png")); // 添加退出图标

    // 数据库菜单
    databaseMenu = menuBar->addMenu(tr("数据库(&D)"));
    QAction *refreshAction = databaseMenu->addAction(tr("刷新"), this, &MainWindow::onRefreshTables);
    QAction *vacuumAction = databaseMenu->addAction(tr("压缩数据库"), this, &MainWindow::onVacuumDatabase);
    vacuumAction->setIcon(QIcon(":/icons/database_vacuum.png"));
    vacuumAction->setShortcut(QKeySequence("Ctrl+Shift+V"));

    // 菜单分隔符
    databaseMenu->addSeparator();

    // 对象菜单（原表管理菜单）
    tableMenu = menuBar->addMenu(tr("对象(&O)"));
    tableMenu->addAction(tr("新建表"), this, &MainWindow::onCreateTable);
    tableMenu->addAction(tr("删除表"), this, &MainWindow::onDropTable);
    // tableMenu->addAction(tr("重命名表"), this, &MainWindow::onRenameTable);
    tableMenu->addAction(tr("清空表"), this, &MainWindow::onClearTable);

    // 视图菜单
    viewMenu = menuBar->addMenu(tr("视图(&V)"));

    // 添加调试信息
    qDebug() << "开始初始化视图菜单...";

    // 添加树形图选项（改为可勾选）
    QAction *treeAction = viewMenu->addAction(tr("树形图"));
    treeAction->setCheckable(true);
    treeAction->setChecked(true);

    // 创建自定义复选框图标
    QPixmap checkedPixmap(16, 16);
    checkedPixmap.fill(Qt::transparent);
    QPainter checkedPainter(&checkedPixmap);
    checkedPainter.setPen(QPen(Qt::black, 2));
    checkedPainter.drawRect(2, 2, 12, 12);
    checkedPainter.drawLine(4, 8, 7, 11);
    checkedPainter.drawLine(7, 11, 12, 4);
    QIcon checkedIcon(checkedPixmap);

    QPixmap uncheckedPixmap(16, 16);
    uncheckedPixmap.fill(Qt::transparent);
    QPainter uncheckedPainter(&uncheckedPixmap);
    uncheckedPainter.setPen(QPen(Qt::black, 2));
    uncheckedPainter.drawRect(2, 2, 12, 12);
    QIcon uncheckedIcon(uncheckedPixmap);

    qDebug() << "创建复选框图标完成";

    // 设置树形图图标
    treeAction->setIcon(checkedIcon);

    // 连接树形图切换信号
    connect(treeAction, &QAction::triggered, [this, treeAction, checkedIcon, uncheckedIcon](bool checked) {
        qDebug() << "树形图状态改变:" << checked;
        if (mainSplitter) {
            // 如果隐藏树形图，则将其大小设为0
            QList<int> sizes = mainSplitter->sizes();
            if (!checked) {
                // 保存当前大小比例，以便恢复时使用
                mainSplitter->setProperty("savedTreeSize", sizes[0]);
                sizes[0] = 0;
            } else {
                // 恢复之前保存的大小，或使用默认值
                int savedSize = mainSplitter->property("savedTreeSize").toInt();
                sizes[0] = savedSize > 0 ? savedSize : 300;
            }
            mainSplitter->setSizes(sizes);
            treeAction->setIcon(checked ? checkedIcon : uncheckedIcon);
            qDebug() << "设置树形图可见性:" << checked;
        }
    });

    viewMenu->addSeparator();

    // 添加标签页选项，带复选框
    QAction *dbTabAction = viewMenu->addAction(tr("数据库标签"));
    QAction *dataTabAction = viewMenu->addAction(tr("数据标签"));
    QAction *ddlTabAction = viewMenu->addAction(tr("DDL标签"));
    QAction *designTabAction = viewMenu->addAction(tr("设计标签"));
    QAction *sqlTabAction = viewMenu->addAction(tr("SQL标签"));

    // 调试信息
    qDebug() << "创建标签页 Actions 完成";

    // 设置复选框
    dbTabAction->setCheckable(true);
    dataTabAction->setCheckable(true);
    ddlTabAction->setCheckable(true);
    designTabAction->setCheckable(true);
    sqlTabAction->setCheckable(true);

    qDebug() << "设置标签页 Checkable 属性完成";

    // 设置初始状态为选中并设置图标
    dbTabAction->setChecked(true);
    dataTabAction->setChecked(true);
    ddlTabAction->setChecked(true);
    designTabAction->setChecked(true);
    sqlTabAction->setChecked(true);

    dbTabAction->setIcon(checkedIcon);
    dataTabAction->setIcon(checkedIcon);
    ddlTabAction->setIcon(checkedIcon);
    designTabAction->setIcon(checkedIcon);
    sqlTabAction->setIcon(checkedIcon);

    qDebug() << "设置标签页初始状态和图标完成";

    // 连接信号和槽
    connect(dbTabAction, &QAction::triggered, [this, dbTabAction, checkedIcon, uncheckedIcon](bool checked) {
        qDebug() << "数据库标签页状态改变:" << checked;
        if (rightTabWidget) {
            rightTabWidget->setTabVisible(0, checked);
            dbTabAction->setIcon(checked ? checkedIcon : uncheckedIcon);
            qDebug() << "设置数据库标签页可见性:" << checked;
        }
    });

    connect(dataTabAction, &QAction::triggered, [this, dataTabAction, checkedIcon, uncheckedIcon](bool checked) {
        qDebug() << "数据标签页状态改变:" << checked;
        if (rightTabWidget) {
            rightTabWidget->setTabVisible(1, checked);
            dataTabAction->setIcon(checked ? checkedIcon : uncheckedIcon);
            qDebug() << "设置数据标签页可见性:" << checked;
        }
    });

    connect(ddlTabAction, &QAction::triggered, [this, ddlTabAction, checkedIcon, uncheckedIcon](bool checked) {
        qDebug() << "DDL标签页状态改变:" << checked;
        if (rightTabWidget) {
            rightTabWidget->setTabVisible(2, checked);
            ddlTabAction->setIcon(checked ? checkedIcon : uncheckedIcon);
            qDebug() << "设置DDL标签页可见性:" << checked;
        }
    });

    connect(designTabAction, &QAction::triggered, [this, designTabAction, checkedIcon, uncheckedIcon](bool checked) {
        qDebug() << "设计标签页状态改变:" << checked;
        if (rightTabWidget) {
            rightTabWidget->setTabVisible(3, checked);
            designTabAction->setIcon(checked ? checkedIcon : uncheckedIcon);
            qDebug() << "设置设计标签页可见性:" << checked;
        }
    });

    connect(sqlTabAction, &QAction::triggered, [this, sqlTabAction, checkedIcon, uncheckedIcon](bool checked) {
        qDebug() << "SQL标签页状态改变:" << checked;
        if (rightTabWidget) {
            rightTabWidget->setTabVisible(4, checked);
            sqlTabAction->setIcon(checked ? checkedIcon : uncheckedIcon);
            qDebug() << "设置SQL标签页可见性:" << checked;
        }
    });

    // SQL菜单
    sqlMenu = menuBar->addMenu(tr("SQL(&S)"));
    QAction *executeAction = sqlMenu->addAction(tr("执行"), this, &MainWindow::onExecuteSQL);
    executeAction->setShortcut(QKeySequence("Ctrl+Return"));
    QAction *executeLineAction = sqlMenu->addAction(tr("执行当前行"), this, &MainWindow::onExecuteCurrentLine);
    executeLineAction->setShortcut(QKeySequence("Ctrl+L"));
    sqlMenu->addSeparator();
    QAction *formatAction = sqlMenu->addAction(tr("格式化SQL"), this, &MainWindow::onFormatSQL);
    formatAction->setShortcut(QKeySequence("Ctrl+Alt+F"));
    sqlMenu->addSeparator();
    QAction *openScriptAction = sqlMenu->addAction(tr("打开SQL脚本"), this, &MainWindow::onOpenSQLScript);
    openScriptAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    QAction *saveScriptAction = sqlMenu->addAction(tr("保存SQL脚本"), this, &MainWindow::onSaveSQLScript);
    saveScriptAction->setShortcut(QKeySequence("Ctrl+S"));
    QAction *saveAsScriptAction = sqlMenu->addAction(tr("SQL脚本另存为"), this, &MainWindow::onSaveAsSQLScript);
    saveAsScriptAction->setShortcut(QKeySequence("Ctrl+Shift+S"));

    // 搜索菜单
    searchMenu = menuBar->addMenu(tr("搜索(&R)"));
    QAction *searchTableAction = searchMenu->addAction(tr("表查找"), this, &MainWindow::onSearchTable);
    searchTableAction->setShortcut(QKeySequence("Ctrl+F"));
    QAction *searchColumnAction = searchMenu->addAction(tr("字段查找"), this, &MainWindow::onSearchColumn);
    searchColumnAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    QAction *searchValueAction = searchMenu->addAction(tr("数值查找"), this, &MainWindow::onSearchValue);
    searchValueAction->setShortcut(QKeySequence("Ctrl+Alt+N"));

    // 工具菜单
    toolsMenu = menuBar->addMenu(tr("工具(&T)"));
    QAction *zoomInAction = toolsMenu->addAction(tr("放大字体"), this, &MainWindow::onZoomIn);
    zoomInAction->setShortcut(QKeySequence("Ctrl++"));
    QAction *zoomOutAction = toolsMenu->addAction(tr("缩小字体"), this, &MainWindow::onZoomOut);
    zoomOutAction->setShortcut(QKeySequence("Ctrl+-"));

    toolsMenu->addSeparator();

    // 帮助菜单
    helpMenu = menuBar->addMenu(tr("帮助(&H)"));

    // 创建可停靠工具栏
    mainToolBar = addToolBar(tr("主工具栏"));
    mainToolBar->setMovable(true);
    mainToolBar->setFloatable(true);


    // 设置工具栏按钮大小
    mainToolBar->setIconSize(QSize(20, 20));
    mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly); // 只显示图标

    // 为已存在的动作添加图标
    QIcon openIcon(":/icons/open_database.png");
    openDbAction->setIcon(openIcon);
    openDbAction->setStatusTip(tr("打开现有数据库"));
    mainToolBar->addAction(openDbAction);

    QIcon newIcon(":/icons/new_database.png");
    newDbAction->setIcon(newIcon);
    newDbAction->setStatusTip(tr("创建新数据库"));
    mainToolBar->addAction(newDbAction);

    // 添加断开连接按钮
    QIcon disconnectIcon(":/icons/disconnect.png");
    QAction *disconnectAction = new QAction(disconnectIcon, tr("断开连接"), this);
    disconnectAction->setStatusTip(tr("断开数据库连接"));
    connect(disconnectAction, &QAction::triggered, this, &MainWindow::onDisconnectDB);
    mainToolBar->addAction(disconnectAction);

    // 添加分隔符
    mainToolBar->addSeparator();

    // 为刷新动作添加图标
    QIcon refreshIcon(":/icons/refresh.png");
    refreshAction->setIcon(refreshIcon);
    refreshAction->setStatusTip(tr("刷新表列表"));
    mainToolBar->addAction(refreshAction);

    // 为压缩数据库动作添加图标
    vacuumAction->setStatusTip(tr("压缩数据库以回收空间"));
    mainToolBar->addAction(vacuumAction);

    // 添加分隔符
    mainToolBar->addSeparator();

    // 为字体缩放动作添加图标
    QIcon zoomInIcon(":/icons/zoom_in.png");
    zoomInAction->setIcon(zoomInIcon);
    zoomInAction->setStatusTip(tr("增大字体大小"));
    mainToolBar->addAction(zoomInAction);

    QIcon zoomOutIcon(":/icons/zoom_out.png");
    zoomOutAction->setIcon(zoomOutIcon);
    zoomOutAction->setStatusTip(tr("减小字体大小"));
    mainToolBar->addAction(zoomOutAction);

    // 添加主题切换按钮
    // 添加退出按钮
    QIcon exitIcon(":/icons/exit.png");
    QAction *toolbarExitAction = new QAction(exitIcon, tr("退出"), this);
    toolbarExitAction->setStatusTip(tr("退出程序"));
    connect(toolbarExitAction, &QAction::triggered, this, &MainWindow::onExit);
    mainToolBar->addAction(toolbarExitAction);

    // 预留动态工具区域（后续可以根据需要动态添加）

    // 创建主分割器
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // 左侧树形结构（30%宽度）
    tableTree = new QTreeWidget(mainSplitter);
    tableTree->setHeaderHidden(true); // 隐藏默认的表头
    tableTree->setContextMenuPolicy(Qt::CustomContextMenu);

    // 创建数据库标题行容器
    QWidget *dbHeaderWidget = new QWidget(this);
    QHBoxLayout *dbHeaderLayout = new QHBoxLayout(dbHeaderWidget);
    dbHeaderLayout->setContentsMargins(5, 5, 5, 5);
    dbHeaderLayout->setSpacing(5);

    // 创建数据库图标标签
    QLabel *dbIconLabel = new QLabel(dbHeaderWidget);
    dbIconLabel->setPixmap(QIcon(":/icons/database_logo.png").pixmap(16, 16));
    dbIconLabel->setFixedSize(16, 16);

    // 创建数据库名称标签
    dbTitleLabel = new QLabel("未连接数据库", dbHeaderWidget);
    dbTitleLabel->setStyleSheet("QLabel { color: black; font-weight: bold; }");

    // 添加到布局
    dbHeaderLayout->addWidget(dbIconLabel);
    dbHeaderLayout->addWidget(dbTitleLabel, 1);

    // 创建垂直布局来包含标题和树形视图
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // 添加数据库标题和树形视图到布局
    leftLayout->addWidget(dbHeaderWidget);
    leftLayout->addWidget(tableTree);

    // 将左侧部件添加到分割器
    mainSplitter->addWidget(leftWidget);

    // 设置树形图的样式
    tableTree->setStyleSheet(
        // 选中项的样式（包括分支区域）
        "QTreeWidget::item:selected, QTreeWidget::branch:adjoins-item:has-siblings:selected,"
        "QTreeWidget::branch:adjoins-item:!has-siblings:selected {"
        "    background-color: rgb(0, 115, 229);"
        "    color: white;"
        "}"

        // 默认背景和间距
        "QTreeWidget::item {"
        "    background-color: #E6E6E6;"
        "    padding-top: 2px;"
        "    padding-bottom: 2px;"
        "    height: 20px;" // 固定项目高度
        "}"
        "QTreeWidget::item:has-children { color: black; }"
        "QTreeWidget::item:!has-children { color: #404040; }"
        "QTreeWidget::branch { background: #E6E6E6; }"

        // 基本竖线样式 - 只显示到最后一个子项
        "QTreeWidget::branch:has-siblings {"
        "    border-left: 1px dotted black;"
        "    margin-left: 15px;"
        "}"

        // 没有兄弟节点的分支 - 不显示延伸的竖线
        "QTreeWidget::branch:!has-siblings {"
        "    border: none;"
        "    margin-left: 15px;"
        "}"

        // 连接到项目的分支（带横线）
        "QTreeWidget::branch:has-siblings:adjoins-item {"
        "    border-left: 1px dotted black;"
        "    border-bottom: 1px dotted black;"
        "    margin-left: 15px;"
        "    padding-left: 10px;"
        "    height: 10px;"    // 减小连接线高度
        "    margin-top: 5px;" // 向下偏移以居中对齐
        "}"

        // 最后一个项目的分支（带横线）
        "QTreeWidget::branch:!has-siblings:adjoins-item {"
        "    border-left: 1px dotted black;"
        "    border-bottom: 1px dotted black;"
        "    margin-left: 15px;"
        "    padding-left: 10px;"
        "    height: 10px;"    // 减小连接线高度
        "    margin-top: 5px;" // 向下偏移以居中对齐
        "}");

    // 设置树形图的连接线样式
    tableTree->setIndentation(40);         // 调整缩进以适应连接线
    tableTree->setAnimated(true);          // 启用动画效果
    tableTree->setRootIsDecorated(true);   // 显示展开/折叠图标
    tableTree->setUniformRowHeights(true); // 统一行高

    // 设置项目之间的间距
    tableTree->setStyleSheet(tableTree->styleSheet() +
                             "QTreeWidget::item {"
                             "    padding-top: 2px;"    // 顶部间距
                             "    padding-bottom: 2px;" // 底部间距
                             "}");

    // 设置自定义代理来处理图标
    tableTree->setItemDelegate(new TreeItemDelegate(tableTree));

    // 创建数据表格
    dataTable = new QTableWidget(this);
    dataTable->setEditTriggers(QTableWidget::NoEditTriggers);   // 禁用直接编辑
    dataTable->setSelectionBehavior(QTableWidget::SelectRows);  // 整行选择
    dataTable->setSelectionMode(QTableWidget::SingleSelection); // 单行选择
    dataTable->setAlternatingRowColors(true);                   // 交替行颜色

    // 显示行号
    dataTable->verticalHeader()->setVisible(true);
    dataTable->verticalHeader()->setDefaultSectionSize(25);
    dataTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    dataTable->verticalHeader()->setSectionsClickable(false); // 禁止点击行号
    dataTable->verticalHeader()->setHighlightSections(false); // 禁止高亮行号

    // 设置表格样式
    dataTable->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #d0d0d0;"
        "    gridline-color: #f0f0f0;"
        "    background-color: white;"
        "}"
        "QTableWidget::item {"
        "    padding: 5px;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #0078d7;"
        "    color: white;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f0f0f0;"
        "    padding: 4px;"
        "    border: 1px solid #d0d0d0;"
        "}"
        "QTableCornerButton::section {"
        "    background-color: #f0f0f0;"
        "    border: 1px solid #d0d0d0;"
        "}");

    // 设置表格属性
    dataTable->setShowGrid(true);
    dataTable->setGridStyle(Qt::SolidLine);
    dataTable->horizontalHeader()->setHighlightSections(false);
    dataTable->verticalHeader()->setVisible(false);
    dataTable->setFrameShape(QFrame::StyledPanel);
    dataTable->setFrameShadow(QFrame::Sunken);

    // 设置列宽模式
    dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); // 允许手动调整列宽
    dataTable->horizontalHeader()->setStretchLastSection(true);                    // 最后一列自动拉伸

    // 设置行高
    dataTable->verticalHeader()->setDefaultSectionSize(28);

    // 设置列宽默认值为内容宽度
    dataTable->resizeColumnsToContents();

    // 右侧标签页
    rightTabWidget = new QTabWidget(mainSplitter);
    rightTabWidget->setTabPosition(QTabWidget::North);
    rightTabWidget->setMovable(true);
    rightTabWidget->setTabsClosable(true);
    rightTabWidget->setDocumentMode(true);
    rightTabWidget->setElideMode(Qt::ElideRight);
    rightTabWidget->setUsesScrollButtons(true);
    rightTabWidget->setTabBarAutoHide(false);
    
    // 创建自定义的关闭按钮图标
    QPixmap closePixmap(16, 16);
    closePixmap.fill(Qt::transparent);
    QPainter closePainter(&closePixmap);
    closePainter.setRenderHint(QPainter::Antialiasing);
    closePainter.setPen(QPen(QColor(120, 120, 120), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    closePainter.drawLine(4, 4, 12, 12);
    closePainter.drawLine(12, 4, 4, 12);
    QIcon closeIcon(closePixmap);
    
    // 自定义标签页样式，使用内存中创建的图标
    rightTabWidget->setStyleSheet(R"(
        QTabWidget::pane { 
            border: 1px solid #C0C0C0; 
        }
        QTabBar::tab {
            background: #F0F0F0;
            border: 1px solid #C0C0C0;
            border-bottom-color: #C0C0C0;
            border-top-left-radius:4px;
            border-top-right-radius: 4px;
            min-width: 8ex;
            padding: 2px 8px;
            padding-right: 20px; /* 为关闭按钮留出空间 */
            margin-right: 1px;
        }
        QTabBar::tab:selected {
            background: #FFFFFF;
            border-bottom-color: #FFFFFF;
        }
        QTabBar::tab:hover {
            background: #E0E0E0;
        }
    )");
    
    // 为每个标签页设置自定义关闭按钮图标
    for (int i = 0; i < rightTabWidget->count(); i++) {
        rightTabWidget->tabBar()->setTabButton(i, QTabBar::RightSide, createTabCloseButton());
    }

    qDebug() << "创建右侧标签页区域完成";

    // 创建并添加所有标签页
    databaseTab = new QWidget();
    rightTabWidget->addTab(databaseTab, "数据库");
    
    QWidget *dataTab = new QWidget();
    rightTabWidget->addTab(dataTab, "数据");
    
    QWidget *ddlTab = new QWidget();
    rightTabWidget->addTab(ddlTab, "DDL");
    
    QWidget *designTab = new QWidget();
    rightTabWidget->addTab(designTab, "设计");
    
    QWidget *sqlTab = new QWidget();
    rightTabWidget->addTab(sqlTab, "SQL");

    // 立即设置所有标签页为可见并设置自定义关闭按钮
    for(int i = 0; i < rightTabWidget->count(); i++) {
        rightTabWidget->setTabVisible(i, true);
        rightTabWidget->tabBar()->setTabButton(i, QTabBar::RightSide, createTabCloseButton());
        qDebug() << "设置标签页" << i << "可见性为: true 并应用自定义关闭按钮";
    }

    // 同步菜单项的选中状态
    QList<QAction*> viewActions = viewMenu->actions();
    for(int i = 2; i < viewActions.size(); i++) {
        if(QAction* action = viewActions[i]) {
            action->setChecked(true);
            qDebug() << "设置菜单项" << action->text() << "选中状态为: true";
        }
    }

    // 创建数据库视图标签页
    QVBoxLayout *databaseLayout = new QVBoxLayout(databaseTab);

    // 创建数据库信息面板
    QGroupBox *dbInfoGroup = new QGroupBox("数据库信息");
    QVBoxLayout *dbInfoLayout = new QVBoxLayout(dbInfoGroup);

    // 创建信息标签
    QLabel *dbNameLabel = new QLabel("数据库名称: ");
    QLabel *dbVersionLabel = new QLabel("数据库版本: ");
    QLabel *dbOverviewLabel = new QLabel("数据库概述: ");
    QLabel *totalPageLabel = new QLabel("总页数: ");
    QLabel *freePageLabel = new QLabel("空闲页: ");
    QLabel *memPageCostLabel = new QLabel(QString("内存页成本: %1").arg(MEM_PAGE_COST));
    QLabel *randomPageCostLabel = new QLabel(QString("随机页成本: %1").arg(RANDOM_PAGE_COST));

    // 添加标签到布局
    dbInfoLayout->addWidget(dbNameLabel);
    dbInfoLayout->addWidget(dbVersionLabel);
    dbInfoLayout->addWidget(dbOverviewLabel);
    dbInfoLayout->addWidget(totalPageLabel);
    dbInfoLayout->addWidget(freePageLabel);
    dbInfoLayout->addWidget(memPageCostLabel);
    dbInfoLayout->addWidget(randomPageCostLabel);

    // 添加信息面板到数据库布局
    databaseLayout->addWidget(dbInfoGroup);
    databaseLayout->addWidget(dataTable);

    // 默认隐藏数据库信息面板
    dbInfoGroup->setVisible(false);

    // 数据标签页
    QVBoxLayout *dataLayout = new QVBoxLayout(dataTab);

    // 添加功能按钮栏
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // 创建图标按钮
    QToolButton *firstRecordBtn = new QToolButton(this);
    firstRecordBtn->setObjectName("firstRecordBtn");
    qDebug() << "创建第一条记录按钮，对象名称:" << firstRecordBtn->objectName();
    firstRecordBtn->setIcon(QIcon(":/icons/first.png"));
    firstRecordBtn->setToolTip(tr("显示第一条记录"));
    QToolButton *prevRecordBtn = new QToolButton(this);
    QToolButton *nextRecordBtn = new QToolButton(this);
    QToolButton *lastRecordBtn = new QToolButton(this);
    QToolButton *newRecordBtn = new QToolButton(this);
    QToolButton *deleteRecordBtn = new QToolButton(this);
    QToolButton *editRecordBtn = new QToolButton(this);
    QToolButton *refreshBtn = new QToolButton(this);
    refreshBtn->setObjectName("refreshBtn");
    refreshBtn->setIcon(QIcon(":/icons/refresh.png"));
    refreshBtn->setToolTip(tr("刷新数据"));

    // 添加自动刷新控制
    QCheckBox *autoRefreshCheck = new QCheckBox("自动刷新", this);
    autoRefreshCheck->setToolTip("启用自动刷新数据");
    QSpinBox *refreshIntervalSpin = new QSpinBox(this);
    refreshIntervalSpin->setRange(1, 60);
    refreshIntervalSpin->setValue(5);
    refreshIntervalSpin->setSuffix(" 秒");
    refreshIntervalSpin->setToolTip("刷新间隔时间");

    // 创建定时器
    QTimer *refreshTimer = new QTimer(this);
    refreshTimer->setInterval(refreshIntervalSpin->value() * 1000);

    // 连接自动刷新控制信号
    connect(autoRefreshCheck, &QCheckBox::toggled, [refreshTimer, refreshIntervalSpin](bool checked)
            {
        if (checked) {
            refreshTimer->start();
        } else {
            refreshTimer->stop();
        } });

    connect(refreshIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), [refreshTimer](int value)
            { refreshTimer->setInterval(value * 1000); });

    // 连接定时器超时信号
    connect(refreshTimer, &QTimer::timeout, [this]()
            {
        if (!currentTable.isEmpty()) {
            onTableSelected(tableTree->currentItem(), 0);
        } });

    // 设置按钮属性
    firstRecordBtn->setText("第一条记录");
    prevRecordBtn->setText("上一条记录");
    nextRecordBtn->setText("下一条记录");
    lastRecordBtn->setText("最后一条记录");
    newRecordBtn->setText("新建记录");
    deleteRecordBtn->setText("删除记录");
    editRecordBtn->setText("编辑记录");
    refreshBtn->setText("刷新");

    // 设置图标
    firstRecordBtn->setIcon(QIcon(":/icons/first_record.png"));
    prevRecordBtn->setIcon(QIcon(":/icons/prev_record.png"));
    nextRecordBtn->setIcon(QIcon(":/icons/next_record.png"));
    lastRecordBtn->setIcon(QIcon(":/icons/last_record.png"));
    newRecordBtn->setIcon(QIcon(":/icons/new_record.png"));
    deleteRecordBtn->setIcon(QIcon(":/icons/delete_record.png"));
    editRecordBtn->setIcon(QIcon(":/icons/edit.png"));
    refreshBtn->setIcon(QIcon(":/icons/cached.png"));

    // 设置按钮样式
    QString buttonStyle = "QToolButton { padding: 5px; margin: 2px; }";
    firstRecordBtn->setStyleSheet(buttonStyle);
    prevRecordBtn->setStyleSheet(buttonStyle);
    nextRecordBtn->setStyleSheet(buttonStyle);
    lastRecordBtn->setStyleSheet(buttonStyle);
    newRecordBtn->setStyleSheet(buttonStyle);
    deleteRecordBtn->setStyleSheet(buttonStyle);
    editRecordBtn->setStyleSheet(buttonStyle);
    refreshBtn->setStyleSheet(buttonStyle);

    // 设置工具提示
    firstRecordBtn->setToolTip("第一条记录");
    prevRecordBtn->setToolTip("上一条记录");
    nextRecordBtn->setToolTip("下一条记录");
    lastRecordBtn->setToolTip("最后一条记录");
    newRecordBtn->setToolTip("新建记录");
    deleteRecordBtn->setToolTip("删除记录");
    editRecordBtn->setToolTip("编辑记录");
    refreshBtn->setToolTip("刷新数据");

    // 添加按钮到布局
    buttonLayout->addWidget(firstRecordBtn);
    buttonLayout->addWidget(prevRecordBtn);
    buttonLayout->addWidget(nextRecordBtn);
    buttonLayout->addWidget(lastRecordBtn);
    buttonLayout->addWidget(newRecordBtn);
    buttonLayout->addWidget(deleteRecordBtn);
    buttonLayout->addWidget(editRecordBtn);
    buttonLayout->addWidget(refreshBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(autoRefreshCheck);
    buttonLayout->addWidget(refreshIntervalSpin);

    // 添加按钮布局到数据标签页
    dataLayout->addLayout(buttonLayout);

    // 添加数据表格
    dataLayout->addWidget(dataTable);

    // 连接按钮信号
    connect(firstRecordBtn, &QToolButton::clicked, this, [this]()
            {
        if (dataTable->rowCount() > 0) {
            dataTable->selectRow(0);
        } });

    connect(prevRecordBtn, &QToolButton::clicked, this, [this]()
            {
        int currentRow = dataTable->currentRow();
        if (currentRow > 0) {
            dataTable->selectRow(currentRow - 1);
        } });

    connect(nextRecordBtn, &QToolButton::clicked, this, [this]()
            {
        int currentRow = dataTable->currentRow();
        if (currentRow < dataTable->rowCount() - 1) {
            dataTable->selectRow(currentRow + 1);
        } });

    connect(lastRecordBtn, &QToolButton::clicked, this, [this]()
            {
        if (dataTable->rowCount() > 0) {
            dataTable->selectRow(dataTable->rowCount() - 1);
        } });

    connect(newRecordBtn, &QToolButton::clicked, this, &MainWindow::onAddRow);
    connect(deleteRecordBtn, &QToolButton::clicked, this, &MainWindow::onDeleteRow);
    connect(editRecordBtn, &QToolButton::clicked, this, &MainWindow::onEditRow);
    connect(refreshBtn, &QToolButton::clicked, this, [this]()
            {
        qDebug() << "刷新按钮被点击";
        if (!currentTable.isEmpty()) {
            // 直接调用onTableSelected来刷新数据
            QTreeWidgetItem* currentItem = tableTree->currentItem();
            if (currentItem) {
                onTableSelected(currentItem, 0);
            }
        } });

    // DDL标签页
    QVBoxLayout *ddlLayout = new QVBoxLayout(ddlTab);
    ddlEditor = new QTextEdit();
    ddlEditor->setReadOnly(true);
    ddlLayout->addWidget(ddlEditor);

    // 为DDL编辑器添加高亮器
    SqlHighlighter *ddlHighlighter = new SqlHighlighter(ddlEditor->document());
    ddlHighlighter->setTheme("Light");

    // 设计标签页
    QVBoxLayout *designLayout = new QVBoxLayout(designTab);

    // 创建表格来显示表结构
    QTableWidget *designTable = new QTableWidget();
    designTable->setColumnCount(11); // 设置列数
    designTable->setHorizontalHeaderLabels({"序号", "列名", "数据类型", "长度", "精度", "主键",
                                            "允许空", "最小值", "最大值", "默认值", "唯一性"});
    designTable->setEditTriggers(QTableWidget::NoEditTriggers);
    designTable->setSelectionBehavior(QTableWidget::SelectRows);
    designTable->setAlternatingRowColors(true);

    // 设置表格样式
    designTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            gridline-color: #e9ecef;
        }
        QTableWidget::item {
            padding: 8px;
            border-bottom: 1px solid #e9ecef;
            color: #495057;
        }
        QTableWidget::item:selected {
            background-color: #e9ecef;
            color: #212529;
        }
        QHeaderView::section {
            background-color: #f8f9fa;
            padding: 10px;
            border: none;
            border-bottom: 2px solid #dee2e6;
            font-weight: bold;
            color: #495057;
        }
        QTableWidget::item:hover {
            background-color: #f8f9fa;
        }
    )");

    // 设置列宽
    designTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    designTable->horizontalHeader()->setMinimumSectionSize(80);
    designTable->horizontalHeader()->setDefaultSectionSize(100);

    // 设置行高
    designTable->verticalHeader()->setDefaultSectionSize(32);
    designTable->verticalHeader()->setVisible(false);

    // 设置表格属性
    designTable->setShowGrid(true);
    designTable->setGridStyle(Qt::SolidLine);
    designTable->setFrameShape(QFrame::StyledPanel);
    designTable->setFrameShadow(QFrame::Sunken);

    designLayout->addWidget(designTable);

    // 添加更新设计视图的函数
    auto updateDesignView = [this, designTable](const QString &tableName)
    {
        if (!db || tableName.isEmpty())
            return;

        // 清空表格
        designTable->clearContents();
        designTable->setRowCount(0);

        // 查询表结构
        SQLResult result;
        QString sql = QString("SELECT * FROM schema WHERE tableName = '%1' ORDER BY columnIndex").arg(tableName);
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);

        if (rc != 0)
        {
            if (errmsg)
                free(errmsg);
            return;
        }

        // 设置行数
        designTable->setRowCount(result.rows.size() - 3);

        // 填充数据
        for (int i = 0; i < result.rows.size(); ++i)
        {

            const QStringList &row = result.rows[i];
            if (row[1] == "createTime" || row[1] == "updateTime" || row[1] == "deleteTime")
            {
                continue;
            }
            // 序号
            QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(row[3].toInt() + 1));
            designTable->setItem(i, 0, idItem);

            // 列名
            QTableWidgetItem *nameItem = new QTableWidgetItem(row[1]);
            designTable->setItem(i, 1, nameItem);

            // 数据类型
            int typeId = row[4].toInt();
            QString typeStr = getFieldTypeName(typeId);
            QTableWidgetItem *typeItem = new QTableWidgetItem(typeStr);
            designTable->setItem(i, 2, typeItem);

            // 长度（对于VARCHAR类型）
            QTableWidgetItem *lengthItem = new QTableWidgetItem();
            if (row[4].toInt() == 3)
            {
                lengthItem->setText(QString::number((int)(row[7].toFloat() - row[6].toFloat()))); // columnLength
            }
            designTable->setItem(i, 3, lengthItem);

            // 精度（暂时为空）
            QTableWidgetItem *scaleItem = new QTableWidgetItem();
            designTable->setItem(i, 4, scaleItem);

            // 主键
            QTableWidgetItem *pkItem = new QTableWidgetItem(row[8] == "1" ? "是" : "否");
            designTable->setItem(i, 5, pkItem);

            // 允许空
            QTableWidgetItem *nullableItem = new QTableWidgetItem(row[5] == "1" ? "是" : "否");
            designTable->setItem(i, 6, nullableItem);

            // 最小值
            QTableWidgetItem *minItem = new QTableWidgetItem(row[6]);
            if (typeId == FIELDTYPE_INTEGER)
            {
                minItem->setText("-2147483648");
            }
            else if (typeId == FIELDTYPE_REAL)
            {
                minItem->setText("-1.7976931348623157e+308");
            }
            designTable->setItem(i, 7, minItem);

            // 最大值
            QTableWidgetItem *maxItem = new QTableWidgetItem(row[7]);
            if (typeId == FIELDTYPE_INTEGER)
            {
                maxItem->setText("2147483647");
            }
            else if (typeId == FIELDTYPE_REAL)
            {
                maxItem->setText("1.7976931348623157e+308");
            }
            designTable->setItem(i, 8, maxItem);

            // 默认值（暂时为空）
            QTableWidgetItem *defaultItem = new QTableWidgetItem();
            designTable->setItem(i, 9, defaultItem);

            // 唯一性（暂时为空）
            QTableWidgetItem *uniqueItem = new QTableWidgetItem();
            designTable->setItem(i, 10, uniqueItem);
        }
        designTable->resizeColumnsToContents();
    };

    // 连接表选择信号到更新设计视图
    connect(tableTree, &QTreeWidget::itemClicked, [updateDesignView](QTreeWidgetItem *item)
            {
        if (item && !item->parent()) {  // 只处理表项，不处理列项
            updateDesignView(item->text(0));
        } });

    // SQL标签页
    QVBoxLayout *sqlLayout = new QVBoxLayout(sqlTab);

    // 创建分割器
    QSplitter *sqlSplitter = new QSplitter(Qt::Vertical, sqlTab);

    // 上半部分：SQL输入区域
    QWidget *sqlInputWidget = new QWidget();
    QVBoxLayout *sqlInputLayout = new QVBoxLayout(sqlInputWidget);

    // SQL编辑器
    sqlEditor = new QTextEdit();
    sqlEditor->setPlaceholderText(tr("在此输入SQL语句..."));
    sqlEditor->setStyleSheet(R"(
        QTextEdit {
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 14px;
            line-height: 1.5;
        }
        QTextEdit:focus {
            border: 1px solid #80bdff;
            background-color: #ffffff;
        }
    )");
    sqlInputLayout->addWidget(sqlEditor);

    // SQL执行按钮
    // QPushButton *executeButton = new QPushButton(tr("执行SQL"));
    // executeButton->setFixedWidth(80);
    // executeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // connect(executeButton, &QPushButton::clicked, this, &MainWindow::onExecuteSQL);

    // 创建一个水平布局来放置按钮
    // QHBoxLayout *sqlButtonLayout = new QHBoxLayout();
    // sqlButtonLayout->addWidget(executeButton);
    // sqlButtonLayout->addStretch();
    // sqlInputLayout->addLayout(sqlButtonLayout);

    // 下半部分：结果显示区域
    QWidget *sqlResultWidget = new QWidget();
    QVBoxLayout *sqlResultLayout = new QVBoxLayout(sqlResultWidget);

    // SQL结果显示区域
    sqlResultDisplay = new QTableWidget();
    sqlResultDisplay->setEditTriggers(QTableWidget::DoubleClicked |
                                      QTableWidget::EditKeyPressed |
                                      QTableWidget::AnyKeyPressed);
    sqlResultDisplay->setSelectionBehavior(QTableWidget::SelectItems);
    sqlResultDisplay->setSelectionMode(QTableWidget::SingleSelection);
    sqlResultDisplay->setAlternatingRowColors(true);
    sqlResultDisplay->setStyleSheet(R"(
        QTableWidget {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            gridline-color: #dee2e6;
        }
        QTableWidget::item {
            padding: 6px;
            border-bottom: 1px solid #dee2e6;
        }
        QTableWidget::item:selected {
            background-color: #e9ecef;
            color: #212529;
        }
        QHeaderView::section {
            background-color: #f8f9fa;
            padding: 8px;
            border: none;
            border-bottom: 2px solid #dee2e6;
            font-weight: bold;
        }
        QTableWidget::item:hover {
            background-color: #f8f9fa;
        }
    )");
    sqlResultLayout->addWidget(sqlResultDisplay);

    // 连接表格编辑信号
    connect(sqlResultDisplay, &QTableWidget::itemChanged,
            this, &MainWindow::onCellEdited);

    // 添加输入和结果显示区域到分割器
    sqlSplitter->addWidget(sqlInputWidget);
    sqlSplitter->addWidget(sqlResultWidget);

    // 设置分割器初始比例（10:0）
    sqlSplitter->setStretchFactor(0, 10);
    sqlSplitter->setStretchFactor(1, 0);

    // 添加分割器到主布局
    sqlLayout->addWidget(sqlSplitter);

    // 底部按钮栏
    QHBoxLayout *bottomButtonLayout = new QHBoxLayout();

    // 创建按钮
    QToolButton *firstLineBtn = new QToolButton(this);
    QToolButton *prevLineBtn = new QToolButton(this);
    QToolButton *nextLineBtn = new QToolButton(this);
    QToolButton *lastLineBtn = new QToolButton(this);
    QToolButton *openScriptBtn = new QToolButton(this);
    QPushButton *executeCurrentBtn = new QPushButton("执行当前行", this); // 改为QPushButton
    QPushButton *executeAllBtn = new QPushButton("执行全部SQL", this);    // 新增执行全部按钮

    // 设置按钮属性
    firstLineBtn->setText("第一行");
    prevLineBtn->setText("上一行");
    nextLineBtn->setText("下一行");
    lastLineBtn->setText("最后一行");
    openScriptBtn->setText("打开脚本");

    // 设置图标
    firstLineBtn->setIcon(QIcon(":/icons/first_record.png"));
    prevLineBtn->setIcon(QIcon(":/icons/prev_record.png"));
    nextLineBtn->setIcon(QIcon(":/icons/next_record.png"));
    lastLineBtn->setIcon(QIcon(":/icons/last_record.png"));
    openScriptBtn->setIcon(QIcon(":/icons/new_record.png"));

    // 设置按钮样式
    QString sqlButtonStyle = "QToolButton { padding: 5px; margin: 2px; }";
    firstLineBtn->setStyleSheet(sqlButtonStyle);
    prevLineBtn->setStyleSheet(sqlButtonStyle);
    nextLineBtn->setStyleSheet(sqlButtonStyle);
    lastLineBtn->setStyleSheet(sqlButtonStyle);
    openScriptBtn->setStyleSheet(sqlButtonStyle);

    // 设置文字按钮样式
    QString textButtonStyle = "QPushButton { padding: 5px 10px; margin: 2px; }";
    executeCurrentBtn->setStyleSheet(textButtonStyle);
    executeAllBtn->setStyleSheet(textButtonStyle);

    // 设置工具提示
    firstLineBtn->setToolTip("移动到第一行");
    prevLineBtn->setToolTip("移动到上一行");
    nextLineBtn->setToolTip("移动到下一行");
    lastLineBtn->setToolTip("移动到最后一行");
    openScriptBtn->setToolTip("打开SQL脚本");
    executeCurrentBtn->setToolTip("执行当前光标所在行的SQL语句");
    executeAllBtn->setToolTip("执行所有SQL语句");

    // 添加按钮到布局
    bottomButtonLayout->addWidget(firstLineBtn);
    bottomButtonLayout->addWidget(prevLineBtn);
    bottomButtonLayout->addWidget(nextLineBtn);
    bottomButtonLayout->addWidget(lastLineBtn);
    bottomButtonLayout->addWidget(openScriptBtn);
    bottomButtonLayout->addStretch();
    bottomButtonLayout->addWidget(executeCurrentBtn);
    bottomButtonLayout->addWidget(executeAllBtn);

    // 连接按钮信号
    connect(firstLineBtn, &QToolButton::clicked, this, [this]()
            {
        if (sqlEditor) {
            QTextCursor cursor = sqlEditor->textCursor();
            cursor.movePosition(QTextCursor::Start);
            sqlEditor->setTextCursor(cursor);
        } });

    connect(prevLineBtn, &QToolButton::clicked, this, [this]()
            {
        if (sqlEditor) {
            QTextCursor cursor = sqlEditor->textCursor();
            cursor.movePosition(QTextCursor::Up);
            sqlEditor->setTextCursor(cursor);
        } });

    connect(nextLineBtn, &QToolButton::clicked, this, [this]()
            {
        if (sqlEditor) {
            QTextCursor cursor = sqlEditor->textCursor();
            cursor.movePosition(QTextCursor::Down);
            sqlEditor->setTextCursor(cursor);
        } });

    connect(lastLineBtn, &QToolButton::clicked, this, [this]()
            {
        if (sqlEditor) {
            QTextCursor cursor = sqlEditor->textCursor();
            cursor.movePosition(QTextCursor::End);
            sqlEditor->setTextCursor(cursor);
        } });

    connect(openScriptBtn, &QToolButton::clicked, this, &MainWindow::onOpenSQLScript);
    connect(executeCurrentBtn, &QPushButton::clicked, this, &MainWindow::onExecuteCurrentLine);
    connect(executeAllBtn, &QPushButton::clicked, this, &MainWindow::onExecuteSQL);

    // 添加底部按钮栏到主布局
    sqlLayout->addLayout(bottomButtonLayout);

    // 设置分割器初始比例（30:70）
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 7);
    
    // 尝试从设置中恢复分割器位置
    QSettings settings(settingsFilePath, QSettings::IniFormat);
    settings.beginGroup("Splitter");
    
    // 首先检查是否需要从这些设置恢复分割器大小
    if(settings.contains("Sizes"))
    {
        QList<int> savedSizes = settings.value("Sizes").value<QList<int>>();
        if(savedSizes.size() == 2) 
        {
            qDebug() << "恢复分割器位置:" << savedSizes;
            mainSplitter->setSizes(savedSizes);
        }
    }
    
    // 然后检查树形图的可见性
    bool treeVisible = property("settingsTreeVisible").toBool();
    int savedTreeSize = property("settingsSavedTreeSize").toInt();
    
    if (!treeVisible && savedTreeSize > 0) {
        // 如果树形图应该隐藏，记录保存的大小并设置大小为0
        mainSplitter->setProperty("savedTreeSize", savedTreeSize);
        QList<int> sizes = mainSplitter->sizes();
        sizes[0] = 0;
        mainSplitter->setSizes(sizes);
        
        // 同时更新视图菜单中的状态
        QList<QAction*> actions = viewMenu->actions();
        if (!actions.isEmpty()) {
            QAction* treeAction = actions.first(); // 第一个动作应该是树形图
            if (treeAction) {
                treeAction->setChecked(false);
                
                // 更新图标
                QPixmap uncheckedPixmap(16, 16);
                uncheckedPixmap.fill(Qt::transparent);
                QPainter uncheckedPainter(&uncheckedPixmap);
                uncheckedPainter.setPen(QPen(Qt::black, 2));
                uncheckedPainter.drawRect(2, 2, 12, 12);
                QIcon uncheckedIcon(uncheckedPixmap);
                
                treeAction->setIcon(uncheckedIcon);
            }
        }
    }
    
    settings.endGroup();
    
    // 恢复标签页的可见状态
    settings.beginGroup("Tabs");
    for (int i = 0; i < rightTabWidget->count(); ++i)
    {
        bool visible = settings.value(QString("Tab%1Visible").arg(i), true).toBool();
        rightTabWidget->setTabVisible(i, visible);
        
        // 更新菜单项的状态
        QList<QAction*> actions = viewMenu->actions();
        if (actions.size() > i + 2) { // +2是因为有树形图和分隔符
            QAction* action = actions[i + 2];
            action->setChecked(visible);
            
            // 更新图标
            QPixmap pixmap(16, 16);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRect(2, 2, 12, 12);
            if (visible) {
                painter.drawLine(4, 8, 7, 11);
                painter.drawLine(7, 11, 12, 4);
            }
            QIcon icon(pixmap);
            action->setIcon(icon);
        }
    }
    settings.endGroup();

    // 状态栏
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    // 创建状态栏标签
    // dbNameLabel = new QLabel("数据库: 未连接", this);
    tableNameLabel = new QLabel("表: 未选择", this);
    dbPathLabel = new QLabel("路径: 未设置", this);

    // 添加标签到状态栏
    // statusBar->addPermanentWidget(dbNameLabel);
    // statusBar->addPermanentWidget(new QLabel(" | ", this));
    statusBar->addPermanentWidget(tableNameLabel);
    statusBar->addPermanentWidget(new QLabel(" | ", this));
    statusBar->addPermanentWidget(dbPathLabel);

    // 初始化其他成员变量
    currentTable.clear();
    currentColumnNames.clear();

    // 连接菜单项信号，确保当标签页通过菜单显示时也应用自定义关闭按钮
    QList<QAction*> menuActions = viewMenu->actions();
    for(int i = 2; i < menuActions.size(); i++) {
        if(QAction* action = menuActions[i]) {
            int tabIndex = i-2;  // 计算对应的标签页索引
            connect(action, &QAction::triggered, [this, tabIndex]() {
                if(rightTabWidget && tabIndex >= 0 && tabIndex < rightTabWidget->count()) {
                    // 当标签页通过菜单变为可见时设置自定义关闭按钮
                    if(rightTabWidget->isTabVisible(tabIndex)) {
                        // 检查是否已经有了关闭按钮
                        if(!rightTabWidget->tabBar()->tabButton(tabIndex, QTabBar::RightSide) ||
                           !rightTabWidget->tabBar()->tabButton(tabIndex, QTabBar::RightSide)->isVisible()) {
                            rightTabWidget->tabBar()->setTabButton(tabIndex, QTabBar::RightSide, createTabCloseButton());
                        }
                    }
                }
            });
        }
    }
    
    // 当标签页可见性变化时检查并应用自定义关闭按钮
    connect(rightTabWidget, &QTabWidget::tabBarClicked, [this](int index) {
        // 确保可见标签页有自定义关闭按钮
        if(rightTabWidget->isTabVisible(index)) {
            QWidget* closeButton = rightTabWidget->tabBar()->tabButton(index, QTabBar::RightSide);
            if(!closeButton || !closeButton->isVisible()) {
                rightTabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, createTabCloseButton());
                qDebug() << "为标签页" << index << "添加了关闭按钮";
            }
        }
    });
}

void MainWindow::initConnections()
{
    // 修改表选择事件，只在点击表项时触发
    connect(tableTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item)
            {
        // 如果点击的是列信息项，则不执行任何操作
        if (item->parent() != nullptr) {
            return;
        }
        onTableSelected(item, 0); });

    // 添加标签页关闭事件处理
    connect(rightTabWidget, &QTabWidget::tabCloseRequested, this, [this](int index)
            {
        // 获取对应的菜单动作
        QAction* action = nullptr;
        switch (index) {
            case 0: action = viewMenu->actions()[2]; break;  // 数据库标签
            case 1: action = viewMenu->actions()[3]; break;  // 数据标签
            case 2: action = viewMenu->actions()[4]; break;  // DDL标签
            case 3: action = viewMenu->actions()[5]; break;  // 设计标签
            case 4: action = viewMenu->actions()[6]; break;  // SQL标签
        }
        
        if (action) {
            // 创建自定义未勾选图标
            QPixmap uncheckedPixmap(16, 16);
            uncheckedPixmap.fill(Qt::transparent);
            QPainter uncheckedPainter(&uncheckedPixmap);
            uncheckedPainter.setPen(QPen(Qt::black, 2));
            uncheckedPainter.drawRect(2, 2, 12, 12);
            QIcon uncheckedIcon(uncheckedPixmap);
            
            // 取消选中状态
            action->setChecked(false);
            // 设置自定义图标
            action->setIcon(uncheckedIcon);
            // 隐藏标签页
            rightTabWidget->setTabVisible(index, false);
            
            // 如果当前显示的标签页被隐藏，则切换到第一个可见的标签页
            if (!rightTabWidget->isTabVisible(rightTabWidget->currentIndex())) {
                for (int i = 0; i < rightTabWidget->count(); ++i) {
                    if (rightTabWidget->isTabVisible(i)) {
                        rightTabWidget->setCurrentIndex(i);
                        break;
                    }
                }
            }
        } });

    // 添加标签页添加事件处理，为新添加的标签页设置自定义关闭按钮
    connect(rightTabWidget->tabBar(), &QTabBar::tabBarDoubleClicked, [this](int index) {
        // 双击标签页标题时，如果该标签隐藏，则显示它
        if (!rightTabWidget->isTabVisible(index)) {
            rightTabWidget->setTabVisible(index, true);
            
            // 更新对应菜单项的状态
            QAction* action = nullptr;
            switch (index) {
                case 0: action = viewMenu->actions()[2]; break;  // 数据库标签
                case 1: action = viewMenu->actions()[3]; break;  // 数据标签
                case 2: action = viewMenu->actions()[4]; break;  // DDL标签
                case 3: action = viewMenu->actions()[5]; break;  // 设计标签
                case 4: action = viewMenu->actions()[6]; break;  // SQL标签
            }
            
            if (action) {
                action->setChecked(true);
                
                // 创建自定义勾选图标
                QPixmap checkedPixmap(16, 16);
                checkedPixmap.fill(Qt::transparent);
                QPainter checkedPainter(&checkedPixmap);
                checkedPainter.setPen(QPen(Qt::black, 2));
                checkedPainter.drawRect(2, 2, 12, 12);
                checkedPainter.drawLine(4, 8, 7, 11);
                checkedPainter.drawLine(7, 11, 12, 4);
                QIcon checkedIcon(checkedPixmap);
                
                action->setIcon(checkedIcon);
            }
        }
    });
    
    // 当标签页可见性改变时，重新设置关闭按钮
    connect(rightTabWidget, &QTabWidget::tabBarClicked, [this](int) {
        // 为所有可见的标签页设置自定义关闭按钮
        for (int i = 0; i < rightTabWidget->count(); i++) {
            if (rightTabWidget->isTabVisible(i) && 
                rightTabWidget->tabBar()->tabButton(i, QTabBar::RightSide) == nullptr) {
                rightTabWidget->tabBar()->setTabButton(i, QTabBar::RightSide, createTabCloseButton());
            }
        }
    });

    // 添加焦点变化事件处理
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *old, QWidget *now)
            {
        if (old == tableTree && now != tableTree) {
            QTreeWidgetItem *currentItem = tableTree->currentItem();
            if (currentItem) {
                currentItem->setSelected(false);
            }
        } else if (now == tableTree) {
            QTreeWidgetItem *currentItem = tableTree->currentItem();
            if (currentItem) {
                currentItem->setSelected(true);
            }
        } });

    // 添加展开事件处理
    connect(tableTree, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item)
            {
        // 无论是否有临时子节点，都尝试加载列信息
        if (item->parent() == nullptr) {
            loadTableColumns(item);
        } });

    connect(dataTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::showTableContextMenu);

    // 连接SQL编辑器的文本变化事件，使用定时器延迟处理
    if (sqlEditor)
    {
        qDebug() << "找到SQL编辑器，正在连接文本变化信号...";

        // 创建定时器用于延迟高亮处理
        QTimer *highlightTimer = new QTimer(this);
        highlightTimer->setSingleShot(true);
        highlightTimer->setInterval(300); // 300ms延迟

        // 连接文本变化信号到定时器
        connect(sqlEditor, &QTextEdit::textChanged, [this, highlightTimer]()
                {
            if (highlightTimer && !highlightTimer->isActive()) {
                highlightTimer->start();
            } });

        // 连接定时器超时信号到高亮处理
        connect(highlightTimer, &QTimer::timeout, [this]()
                {
            if (sqlHighlighter && sqlEditor && sqlEditor->document()) {
                //qDebug() << "重新应用SQL高亮";
                try {
                    sqlHighlighter->rehighlight();
                } catch (const std::exception& e) {
                    qDebug() << "高亮处理异常:" << e.what();
                } catch (...) {
                    qCritical() << "未知高亮异常";
                }
            } });
    }
    else
    {
        qDebug() << "警告：未找到SQL编辑器！";
    }
}

void MainWindow::onConnectDB()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开数据库"), "", tr("数据库文件 (*.db *.dat)"));
    if (fileName.isEmpty())
    {
        return;
    }

    // 如果已经连接了数据库，先关闭它
    if (db)
    {
        GNCDB_close(&db);
        db = nullptr;
    }

    // 通过 dbManager 打开数据库
    if (!dbManager->open(fileName))
    {
        QMessageBox::critical(this, tr("错误"), tr("无法打开数据库"));
        return;
    }

    // 打开数据库
    db = new GNCDB();
    QByteArray pathBytes = fileName.toLocal8Bit();
    int rc = GNCDB_open(&db, pathBytes.data());

    if (rc != 0)
    {
        QMessageBox::critical(this, tr("错误"), tr("无法打开数据库: %1").arg(Rc2Msg::getErrorMsg(rc)));
        dbManager->close(); // 关闭 dbManager 的连接
        delete db;
        db = nullptr;
        return;
    }

    // 更新UI状态
    QFileInfo fileInfo(fileName);
    QString dbName = fileInfo.baseName();

    // 更新状态栏和标签
    statusBar->showMessage(tr("数据库连接成功"));
    if (dbNameLabel)
    {
        dbNameLabel->setText(tr("数据库: %1").arg(dbName));
    }
    if (dbPathLabel)
    {
        dbPathLabel->setText(tr("路径: %1").arg(fileName));
    }
    if (dbTitleLabel)
    {
        dbTitleLabel->setText(dbName);
    }

    // 清空并更新表列表
    tableTree->clear();
    updateTableList();
    updateDatabaseInfo(); // 更新数据库信息
}

void MainWindow::onDisconnectDB()
{
    if (!db)
    {
        QMessageBox::warning(this, "警告", "未连接到数据库");
        return;
    }

    // 先关闭数据库连接
    GNCDB_close(&db);
    db = nullptr;

    // 确保DBManager也关闭连接
    if (dbManager)
    {
        dbManager->close();
    }

    // 清空左侧表树
    tableTree->clear();

    // 清空右侧数据表格
    dataTable->clear();
    dataTable->setRowCount(0);
    dataTable->setColumnCount(0);

    // 清空SQL编辑器
    sqlEditor->clear();

    // 清空DDL编辑器
    if (ddlEditor)
    {
        ddlEditor->clear();
    }

    // 隐藏数据库信息面板
    QGroupBox *dbInfoGroup = nullptr;
    for (QObject *child : databaseTab->children())
    {
        if (child->inherits("QGroupBox"))
        {
            dbInfoGroup = qobject_cast<QGroupBox *>(child);
            break;
        }
    }

    if (dbInfoGroup)
    {
        dbInfoGroup->setVisible(false);
    }

    // 重置状态栏
    statusBar->showMessage("未连接数据库");
    if (dbNameLabel)
    {
        dbNameLabel->setText("数据库: 未连接");
    }
    if (tableNameLabel)
    {
        tableNameLabel->setText("表: 未选择");
    }
    if (dbPathLabel)
    {
        dbPathLabel->setText("路径: 未设置");
    }
    if (dbTitleLabel)
    {
        dbTitleLabel->setText("未连接数据库");
    }
}

void MainWindow::onExecuteSQL()
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }

    QString sql = sqlEditor->toPlainText().trimmed();
    if (sql.isEmpty())
    {
        showError("请输入SQL语句");
        return;
    }

    // 按分号分割SQL语句，并过滤空语句
    QStringList sqlStatements = sql.split(';', Qt::SkipEmptyParts);
    QStringList validStatements;
    for (const QString &stmt : sqlStatements)
    {
        if (!stmt.trimmed().isEmpty())
        {
            validStatements.append(stmt.trimmed());
        }
    }

    if (validStatements.isEmpty())
    {
        showError("没有有效的SQL语句");
        return;
    }

    // 执行每个SQL语句
    for (const QString &stmt : validStatements)
    {
        executeSQLStatement(stmt);
    }

    // 根据结果行数调整分割器比例
    if (sqlResultDisplay->rowCount() > 0)
    {
        QSplitter *splitter = qobject_cast<QSplitter *>(sqlResultDisplay->parent()->parent());
        if (splitter)
        {
            splitter->setStretchFactor(0, 1);
            splitter->setStretchFactor(1, 1);
        }
    }
}

void MainWindow::onTableSelected(QTreeWidgetItem *item, int /*column*/)
{
    if (!db || !item || !dataTable)
        return;

    // 检查是否是表项（没有父项）
    if (!item->parent())
    {
        currentTable = item->text(0);
        tableNameLabel->setText(QString("表: %1").arg(currentTable));
        qDebug() << "选中表:" << currentTable;

        // 更新DDL视图
        updateDDLView(currentTable);

        // 断开之前的信号连接
        disconnect(dataTable, &QTableWidget::cellChanged, this, nullptr);

        // 首先获取表结构信息
        SQLResult schemaResult;
        QString schemaSql = QString("SELECT * FROM schema WHERE tableName = '%1' ORDER BY columnIndex").arg(currentTable);
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, schemaSql.toUtf8().constData(), sqlResultCallback, &schemaResult, &errmsg);

        if (rc != 0)
        {
            QString errorMsg = QString("获取表结构失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
            showError(errorMsg);
            if (errmsg)
                free(errmsg);
            return;
        }

        // 提取列名并过滤系统列
        QStringList columnNames;
        QMap<int, FieldType> columnFieldTypes;
        for (const QStringList &row : schemaResult.rows)
        {
            QString colName = row[1]; // columnName
            if (colName != "rowId" && colName != "createTime" && colName != "updateTime")
            {
                columnNames.append(colName);
                // 获取列的数据类型
                int typeId = row[2].toInt(); // typeId
                columnFieldTypes[columnNames.size() - 1] = static_cast<FieldType>(typeId);
            }
        }

        // 设置列名
        currentColumnNames = columnNames;

        // 设置表格的列
        dataTable->clear();
        dataTable->setColumnCount(columnNames.size());
        dataTable->setHorizontalHeaderLabels(columnNames);

        // 查询表数据
        QString sql = QString("SELECT * FROM %1").arg(currentTable);
        SQLResult dataResult;
        rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &dataResult, &errmsg);

        if (rc != 0)
        {
            QString errorMsg = QString("查询表 %1 失败: %2").arg(currentTable).arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
            showError(errorMsg);
            if (errmsg)
                free(errmsg);
            return;
        }

        // 填充数据
        dataTable->setRowCount(dataResult.rows.size());
        for (int i = 0; i < dataResult.rows.size(); ++i)
        {
            const QStringList &row = dataResult.rows[i];
            for (int j = 0; j < columnNames.size(); ++j)
            {
                QTableWidgetItem *item = new QTableWidgetItem(row[j]);
                dataTable->setItem(i, j, item);
            }
        }

        // 连接单元格编辑信号
        connect(dataTable, &QTableWidget::cellChanged, this, [this, columnFieldTypes](int row, int column)
                {
                    if (!db)
                        return;

                    QStringList values;
                    for (int i = 0; i < dataTable->columnCount(); ++i)
                    {
                        QTableWidgetItem *item = dataTable->item(row, i);
                        values.append(item ? item->text() : QString());
                    }

                    // executeUpdateSQL(currentTable, values, row, columnFieldTypes);
                });
    }
    else
    {
        // 处理列项
        QString columnName = item->text(0);
        qDebug() << "已选择列:" << columnName;
    }
}

void MainWindow::onShowTables()
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }
    updateTableList();
}

void MainWindow::onRefreshTables()
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }

    updateTableList();
    updateDatabaseInfo(); // 更新数据库信息
}

void MainWindow::updateTableList()
{
    if (!db)
    {
        qDebug() << "数据库未连接，无法更新表列表";
        return;
    }

    // 清空现有的表节点
    tableTree->clear();

    // 清空数据表格
    dataTable->clear();
    dataTable->setRowCount(0);
    dataTable->setColumnCount(0);

    // 清空表名列表
    tableList.clear();

    // 使用GNCDB_select函数获取所有表名
    qDebug() << "执行SQL查询: SELECT tableName FROM master;";
    int rc2 = GNCDB_exec(db, "SELECT tableName FROM master", cppCallback, &tableList, NULL);
    if (rc2 != 0)
    {
        QString errorMsg = QString("获取表列表失败，错误代码: %1").arg(rc2);
        qDebug() << errorMsg;
        showError(errorMsg);
        return;
    }

    qDebug() << "SQL查询执行成功，找到" << tableList.size() << "个表";

    // 将表名添加到树形控件中
    for (const TableInfo &table : tableList)
    {
        QTreeWidgetItem *tableItem = new QTreeWidgetItem(tableTree);
        tableItem->setText(0, table.tableName);
        tableItem->setData(0, Qt::UserRole, 1); // 设置节点类型为表
        // 添加一个临时的子项以显示展开箭头
        new QTreeWidgetItem(tableItem);
        qDebug() << "添加表到树形控件:" << table.tableName;
    }

    qDebug() << "表列表更新完成";
}

// 新增：加载表的列信息
void MainWindow::loadTableColumns(QTreeWidgetItem *tableItem)
{
    QString tableName = tableItem->text(0);

    // 清除所有子项（包括临时子项）
    tableItem->takeChildren();

    // 查询表结构
    SQLResult result;
    QString sql = QString("SELECT * FROM schema WHERE tableName = '%1' ORDER BY columnIndex").arg(tableName);
    qDebug() << "[DEBUG] 执行查询表结构的SQL语句:" << sql;
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);

    if (rc == 0)
    {
        qDebug() << "[DEBUG] 查询表结构成功，返回行数:" << result.rows.size();
        // 为每一列创建子节点
        for (const QStringList &row : result.rows)
        {
            qDebug() << "[DEBUG] 查询结果行:" << row;
            if (row.size() < 9)
            {
                qDebug() << "[WARNING] 查询结果行数据不足，跳过：" << row;
                continue;
            }

            QString colName = row[1]; // columnName
            QString colTypeStr = getFieldTypeName(row[4].toInt());
            int colLength = row[5].toInt();
            QString isPrimary = row[8];

            // 跳过系统信息列
            if (colName == "rowId" || colName == "createTime" || colName == "updateTime")
            {
                qDebug() << "[DEBUG] 跳过系统列:" << colName;
                continue;
            }

            // 构建列信息显示文本
            QString columnInfo = QString("%1 (%2)").arg(colName).arg(colTypeStr);
            if (colTypeStr == "VARCHAR")
            {
                columnInfo += QString("(%1)").arg(colLength);
            }
            if (isPrimary == "1")
            {
                columnInfo += " [PK]";
            }

            QTreeWidgetItem *columnItem = new QTreeWidgetItem(tableItem);
            columnItem->setText(0, columnInfo);
            columnItem->setData(0, Qt::UserRole, 2); // 设置节点类型为列

            // 设置列节点为不可选中和不可用
            columnItem->setFlags(columnItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));

            qDebug() << "[DEBUG] 添加列项:" << columnInfo;
        }

        qDebug() << "[DEBUG] 列信息加载完成";
    }
    else
    {
        qDebug() << "[ERROR] 加载表列信息失败，错误码:" << rc << "，错误信息:" << (errmsg ? errmsg : "未知错误");
        if (errmsg)
        {
            free(errmsg);
        }
    }
}

void MainWindow::showTableData(const QString &tableName)
{
    if (!dataTable)
        return;

    qDebug() << "开始显示表格数据:" << tableName;

    // 清空表格
    dataTable->clearContents();
    dataTable->setRowCount(0);

    // 设置当前表名
    dataTable->setProperty("currentTable", tableName);

    // 执行查询
    QString sql = QString("SELECT * FROM %1").arg(tableName);
    executeSQLStatement(sql);

    qDebug() << "数据加载完成，行数:" << dataTable->rowCount();

    // 在数据加载完成后，找到并触发"显示第一条记录"按钮的点击事件
    if (dataTable->rowCount() > 0)
    {
        qDebug() << "准备触发第一条记录按钮点击";
        // 使用QTimer::singleShot确保在数据加载完成后触发
        QTimer::singleShot(100, [this]()
                           {
            qDebug() << "开始查找第一条记录按钮";
            QToolButton *firstRecordBtn = findChild<QToolButton*>("firstRecordBtn");
            if (firstRecordBtn) {
                qDebug() << "找到第一条记录按钮，准备触发点击";
                firstRecordBtn->click();
                qDebug() << "已触发第一条记录按钮点击";
            } else {
                qDebug() << "未找到第一条记录按钮！";
            } });
    }
    else
    {
        qDebug() << "表格中没有数据，不触发按钮点击";
    }
}

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, "错误", message);
    statusBar->showMessage(message);
}

void MainWindow::showCreateTableDialog()
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }

    // 创建对话框
    QDialog dialog(this);
    dialog.setWindowTitle("新建表");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 表名输入
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("表名:", &dialog);
    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    layout->addLayout(nameLayout);

    // 列定义区域
    QWidget *columnsWidget = new QWidget(&dialog);
    QVBoxLayout *columnsLayout = new QVBoxLayout(columnsWidget);
    QList<QHBoxLayout *> columnLayouts;
    QList<QLineEdit *> columnNames;
    QList<QComboBox *> columnTypes;
    QList<QSpinBox *> columnLengths;
    QList<QRadioButton *> primaryKeys;
    QButtonGroup *primaryKeyGroup = new QButtonGroup(&dialog);

    // 添加一列的输入控件
    auto addColumnControls = [&dialog, &columnNames, &columnTypes, &columnLengths, &columnLayouts, &primaryKeys, &primaryKeyGroup, columnsLayout]()
    {
        QHBoxLayout *columnLayout = new QHBoxLayout();

        QLineEdit *colName = new QLineEdit(&dialog);
        colName->setPlaceholderText("列名");

        QComboBox *colType = new QComboBox(&dialog);
        colType->addItems({"Integer", "Real", "VarChar(n)"}); // 显示友好的类型名称

        QSpinBox *colLength = new QSpinBox(&dialog);
        colLength->setRange(1, 255);
        colLength->setValue(64);
        colLength->setEnabled(false);                             // 默认禁用长度输入
        colLength->setButtonSymbols(QAbstractSpinBox::NoButtons); // 默认不显示上下箭头

        QRadioButton *primaryKey = new QRadioButton("主键", &dialog);
        primaryKey->setToolTip("将此列设置为主键");
        primaryKeyGroup->addButton(primaryKey);

        // 当类型改变时更新长度输入框的状态和值
        QObject::connect(colType, &QComboBox::currentTextChanged, [colLength](const QString &text)
                         {
                             if (text == "VarChar(n)") {
                                 colLength->setEnabled(true);
                                 colLength->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
                                 colLength->setValue(32);
                             } else {
                                 colLength->setEnabled(false);  // Integer和Real类型不可编辑长度
                                 colLength->setButtonSymbols(QAbstractSpinBox::NoButtons);
                                 colLength->setValue(64);
                             } });

        columnLayout->addWidget(colName);
        columnLayout->addWidget(colType);
        columnLayout->addWidget(colLength);
        columnLayout->addWidget(primaryKey);

        columnNames.append(colName);
        columnTypes.append(colType);
        columnLengths.append(colLength);
        primaryKeys.append(primaryKey);
        columnLayouts.append(columnLayout);
        columnsLayout->addLayout(columnLayout);
    };

    // 添加第一列
    addColumnControls();
    layout->addWidget(columnsWidget);

    // 添加列按钮
    QPushButton *addColumnBtn = new QPushButton("添加列", &dialog);
    addColumnBtn->setIcon(QIcon(":/icons/new_record.png"));
    // addColumnBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 5px; }");
    layout->addWidget(addColumnBtn);

    // 确定和取消按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(&dialog);
    QPushButton *okButton = buttonBox->addButton("添加表", QDialogButtonBox::AcceptRole);
    QPushButton *cancelButton = buttonBox->addButton("取消", QDialogButtonBox::RejectRole);
    okButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 5px; }");
    cancelButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 5px; }");
    buttonBox->setOrientation(Qt::Horizontal);
    layout->addWidget(buttonBox);

    // 连接添加列按钮
    QObject::connect(addColumnBtn, &QPushButton::clicked, [addColumnControls]()
                     { addColumnControls(); });

    // 连接确定按钮
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [&]()
                     {
        QString tableName = nameEdit->text().trimmed();
        if (tableName.isEmpty()) {
            showError("请输入表名");
            return;
        }

        // 检查是否设置了主键
        if (!primaryKeyGroup->checkedButton()) {
            showError("请设置主键");
            return;
        }

        // 构建CREATE TABLE语句
        QString sql = QString("CREATE TABLE %1 (").arg(tableName);
        QStringList columns;
        
        for (int i = 0; i < columnNames.size(); i++) {
            QString colName = columnNames[i]->text().trimmed();
            QString colType = columnTypes[i]->currentText();
            int length = columnLengths[i]->value();
            
            if (colName.isEmpty()) continue;

            QString sqlType;
            if (colType == "Integer") {
                sqlType = "INT";
            } else if (colType == "Real") {
                sqlType = "FLOAT";//float
            } else if (colType == "VarChar(n)") {
                sqlType = QString("CHAR(%1)").arg(length);
            }

            QString colDef = colName + " " + sqlType;
            if (primaryKeys[i]->isChecked()) {
                colDef += " PRIMARY KEY";
            }
            columns.append(colDef);
        }

        if (columns.isEmpty()) {
            showError("请至少添加一列");
            return;
        }

        sql += columns.join(", ") + ")";
        sql += ";";
        qDebug() << "执行建表SQL:" << sql;

        // 执行建表语句
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
        
        if (rc != 0) {
            QString errorMsg = QString("创建表失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
            showError(errorMsg);
            if (errmsg) free(errmsg);
            return;
        }

        // 刷新表列表
        updateTableList();
        dialog.accept(); });

    // 连接取消按钮
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 显示对话框
    dialog.exec();
}

void MainWindow::onDropTable()
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }

    // 获取当前选中的表
    QTreeWidgetItem *currentItem = tableTree->currentItem();
    if (!currentItem)
    {
        showError("请先选择要删除的表");
        return;
    }

    QString tableName = currentItem->text(0);

    // 确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除表 %1 吗？此操作不可恢复！").arg(tableName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        // 构建DROP TABLE语句
        QString sql = QString("DROP TABLE %1").arg(tableName);
        qDebug() << "执行删表SQL:" << sql;

        // 执行删表语句
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);

        if (rc != 0)
        {
            QString errorMsg = QString("删除表失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
            showError(errorMsg);
            if (errmsg)
                free(errmsg);
            return;
        }

        // 刷新表列表
        updateTableList();
    }
}

void MainWindow::onCreateTable()
{
    showCreateTableDialog();
}

void MainWindow::showTableContextMenu(const QPoint &pos)
{
    if (!db || currentTable.isEmpty())
        return;

    QMenu contextMenu(this);
    QAction *addAction = contextMenu.addAction("添加行");
    QAction *editAction = contextMenu.addAction("编辑行");
    QAction *deleteAction = contextMenu.addAction("删除行");

    QAction *selectedAction = contextMenu.exec(dataTable->viewport()->mapToGlobal(pos));
    if (!selectedAction)
        return;

    if (selectedAction == addAction)
    {
        onAddRow();
    }
    else if (selectedAction == editAction)
    {
        onEditRow();
    }
    else if (selectedAction == deleteAction)
    {
        onDeleteRow();
    }
}

void MainWindow::onAddRow()
{
    if (!db || currentTable.isEmpty())
        return;
    showEditRowDialog();
}

void MainWindow::onEditRow()
{
    if (!db || currentTable.isEmpty())
    {
        QMessageBox::warning(this, "警告", "请先连接数据库并选择表");
        return;
    }

    QList<QTableWidgetItem *> selectedItems = dataTable->selectedItems();
    if (selectedItems.isEmpty())
    {
        QMessageBox::information(this, "提示", "请先选择要编辑的行");
        return;
    }

    int row = selectedItems.first()->row();
    QStringList rowData;
    for (int i = 0; i < dataTable->columnCount(); ++i)
    {
        QTableWidgetItem *item = dataTable->item(row, i);
        rowData.append(item ? item->text() : "");
    }

    // 创建编辑对话框
    QDialog dialog(this);
    dialog.setWindowTitle("编辑行");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 获取表结构信息
    SQLResult schemaResult;
    QString schemaSql = QString("SELECT * FROM schema WHERE tableName = '%1' ORDER BY columnIndex").arg(currentTable);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, schemaSql.toUtf8().constData(), sqlResultCallback, &schemaResult, &errmsg);

    if (rc != 0)
    {
        QString errorMsg = QString("获取表结构失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        showError(errorMsg);
        if (errmsg)
            free(errmsg);
        return;
    }

    // 查找主键列
    QString primaryKeyColumn;
    int primaryKeyIndex = -1;
    FieldType primaryKeyType = FIELDTYPE_INVALID;
    QString primaryKeyValue;

    for (int i = 0; i < schemaResult.rows.size(); ++i)
    {
        const QStringList &schemaRow = schemaResult.rows[i];
        if (schemaRow[8] == "1")
        {                                    // isPrimaryKey
            primaryKeyColumn = schemaRow[1]; // columnName
            primaryKeyIndex = i;
            int colTypeId = schemaRow[4].toInt();                // columnType (编号)
            QString colTypeStr = getFieldTypeName(colTypeId);    // 映射为类型名称
            primaryKeyType = getFieldTypeFromString(colTypeStr); // 获取列的数据类型
            primaryKeyValue = rowData[i];
            break;
        }
    }

    if (primaryKeyColumn.isEmpty())
    {
        QMessageBox::critical(this, "错误", "未找到主键列");
        return;
    }

    // 创建编辑控件
    QList<QLineEdit *> edits;
    QMap<int, FieldType> columnFieldTypes; // 用于存储每列的数据类型

    for (int i = 0; i < schemaResult.rows.size(); ++i)
    {
        const QStringList &schemaRow = schemaResult.rows[i];
        QString columnName = schemaRow[1];                        // columnName
        int colTypeId = schemaRow[4].toInt();                     // columnType (编号)
        QString colTypeStr = getFieldTypeName(colTypeId);         // 映射为类型名称
        FieldType fieldType = getFieldTypeFromString(colTypeStr); // 获取列的数据类型

        // 跳过系统信息列
        if (columnName == "rowId" || columnName == "createTime" || columnName == "updateTime")
        {
            continue;
        }

        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *label = new QLabel(QString("%1 (%2)%3:")
                                       .arg(columnName)
                                       .arg(colTypeStr)
                                       .arg(i == primaryKeyIndex ? " [PK]" : ""),
                                   &dialog);
        QLineEdit *edit = new QLineEdit(&dialog);
        edit->setText(rowData[i]);
        if (i == primaryKeyIndex)
        {
            edit->setReadOnly(true);                           // 主键不可编辑
            edit->setStyleSheet("background-color: #f0f0f0;"); // 设置灰色背景表示不可编辑
        }
        rowLayout->addWidget(label);
        rowLayout->addWidget(edit);
        layout->addLayout(rowLayout);
        edits.append(edit);
        columnFieldTypes[i] = fieldType;
    }

    // 添加按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);

    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, [&]()
            {
        QStringList newValues;
        for (QLineEdit *edit : edits) {
            newValues.append(edit->text());
        }

        // 构建更新语句
        QStringList updateParts;
        for (int i = 0; i < newValues.size(); ++i) {
            // 跳过主键列
            if (i == primaryKeyIndex) {
                continue;
            }
            
            QString columnName = schemaResult.rows[i][1]; // columnName
            QString value = newValues[i];
            
            // 根据数据类型决定是否添加引号
            if (columnFieldTypes[i] == FIELDTYPE_VARCHAR) {
                // 字符串类型添加引号并处理转义
                updateParts << QString("%1 = '%2'").arg(columnName).arg(value.replace("'", "''"));
            } else {
                // 数值类型不添加引号
                updateParts << QString("%1 = %2").arg(columnName).arg(value.isEmpty() ? "NULL" : value);
            }
        }

        // 构建WHERE子句
        QString whereClause;
        if (primaryKeyType == FIELDTYPE_VARCHAR) {
            whereClause = QString("%1 = '%2'").arg(primaryKeyColumn).arg(primaryKeyValue.replace("'", "''"));
        } else {
            whereClause = QString("%1 = %2").arg(primaryKeyColumn).arg(primaryKeyValue);
        }

        QString sql = QString("UPDATE %1 SET %2 WHERE %3")
                         .arg(currentTable)
                         .arg(updateParts.join(", "))
                         .arg(whereClause);

        // 执行更新
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
        qDebug() << "执行更新SQL:" << sql;
        
        if (rc != 0) {
            QMessageBox::critical(this, "错误", QString("更新失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString())));
            if (errmsg) free(errmsg);
        } else {
            // 更新表格显示
            for (int i = 0; i < newValues.size(); ++i) {
                QTableWidgetItem *item = dataTable->item(row, i);
                if (item) {
                    item->setText(newValues[i]);
                }
            }
            dialog.accept();
        } });

    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 显示对话框
    dialog.exec();
}

void MainWindow::onDeleteRow()
{
    if (!db)
    {
        qDebug() << "错误：数据库未连接";
        showError("请先连接数据库");
        return;
    }

    if (currentTable.isEmpty())
    {
        qDebug() << "错误：未选择表";
        showError("请先选择要操作的表");
        return;
    }

    QList<QTableWidgetItem *> selectedItems = dataTable->selectedItems();
    if (selectedItems.isEmpty())
    {
        qDebug() << "错误：未选择要删除的行";
        showError("请先选择要删除的行");
        return;
    }

    int row = selectedItems.first()->row();
    qDebug() << "准备删除行，行号:" << row;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        "确定要删除选中的行吗？此操作不可恢复！",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        qDebug() << "用户确认删除，开始执行删除操作";
        executeDeleteSQL(currentTable, row);
    }
    else
    {
        qDebug() << "用户取消删除操作";
    }
}

QString MainWindow::getFieldTypeName(int typeId)
{
    switch (typeId)
    {
    case FIELDTYPE_INTEGER:
        return "INT";
    case FIELDTYPE_REAL:
        return "REAL";
    case FIELDTYPE_VARCHAR:
        return "CHAR";
    default:
        return "UNKNOWN";
    }
}

void MainWindow::showEditRowDialog(const QStringList &rowData)
{
    QDialog dialog(this);
    dialog.setWindowTitle(rowData.isEmpty() ? "添加行" : "编辑行");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QList<QLineEdit *> valueEdits;
    QMap<int, FieldType> columnFieldTypes; // 用于存储每列的数据类型

    // 获取表的列信息
    SQLResult result;
    QString sql = QString("SELECT * FROM schema WHERE tableName = '%1'").arg(currentTable);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);

    // 添加调试信息
    qDebug() << "执行查询schema语句:" << sql;
    qDebug() << "返回码:" << rc;
    if (errmsg)
    {
        qDebug() << "错误信息:" << errmsg;
    }
    qDebug() << "获取到的表结构:";
    for (const QStringList &row : result.rows)
    {
        qDebug() << row;
    }

    if (rc != 0)
    {
        QString errorMsg = QString("获取表结构失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        showError(errorMsg);
        if (errmsg)
            free(errmsg);
        return;
    }

    // 为每一列创建输入框
    for (const QStringList &row : result.rows)
    {
        QString colName = row[1];                               // columnName
        int colTypeId = row[4].toInt();                         // columnType (编号)
        QString colTypeStr = getFieldTypeName(colTypeId);       // 映射为类型名称
        FieldType colType = getFieldTypeFromString(colTypeStr); // 获取列的数据类型

        // 跳过系统信息列
        if (colName == "rowId" || colName == "createTime" || colName == "updateTime")
        {
            continue;
        }

        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *label = new QLabel(QString("%1 (%2):").arg(colName).arg(colTypeStr), &dialog); // 显示列名和属性类型
        QLineEdit *edit = new QLineEdit(&dialog);

        if (!rowData.isEmpty() && row[3].toInt() < rowData.size())
        { // columnIndex
            edit->setText(rowData[row[3].toInt()]);
        }

        rowLayout->addWidget(label);
        rowLayout->addWidget(edit);
        layout->addLayout(rowLayout);
        valueEdits.append(edit);
        columnFieldTypes[row[3].toInt()] = colType; // 存储列的数据类型
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, [&]()
            {
        QStringList values;
        for (QLineEdit *edit : valueEdits) {
            values.append(edit->text());
        }

        if (rowData.isEmpty()) {
            executeInsertSQL(currentTable, values);
        } else {
            executeUpdateSQL(currentTable, values, dataTable->currentRow(), columnFieldTypes);
        }
        dialog.accept(); });

    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

FieldType MainWindow::getFieldTypeFromString(const QString &typeStr)
{
    if (typeStr == "INT" || typeStr == "INTEGER")
    {
        return FIELDTYPE_INTEGER;
    }
    else if (typeStr == "FLOAT" || typeStr == "REAL")
    {
        return FIELDTYPE_REAL;
    }
    else if (typeStr == "CHAR" || typeStr == "VARCHAR")
    {
        return FIELDTYPE_VARCHAR;
    }
    else
    {
        return FIELDTYPE_INVALID;
    }
}

void MainWindow::executeInsertSQL(const QString &tableName, const QStringList &values)
{
    // 获取表的列信息
    SQLResult result;
    QString schemaQuery = QString("SELECT * FROM schema WHERE tableName = '%1'").arg(tableName);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, schemaQuery.toUtf8().constData(), sqlResultCallback, &result, &errmsg);

    if (rc != 0)
    {
        QString errorMsg = QString("获取表结构失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        showError(errorMsg);
        if (errmsg)
            free(errmsg);
        return;
    }

    // 构建INSERT语句
    QString sql = QString("INSERT INTO %1 (").arg(tableName);
    QStringList columnNames;
    QStringList columnValues;

    int valueIndex = 0; // 用于匹配 values 的索引
    for (const QStringList &row : result.rows)
    {
        QString columnName = row[1]; // columnName
        QString columnType = row[4]; // columnType

        // 跳过系统信息列
        if (columnName == "rowId" || columnName == "createTime" || columnName == "updateTime")
        {
            continue;
        }

        columnNames.append(columnName);

        // 根据数据类型决定是否添加单引号
        int colTypeId = row[4].toInt();                   // columnType (编号)
        QString colTypeStr = getFieldTypeName(colTypeId); // 映射为类型名称
        FieldType fieldType = getFieldTypeFromString(colTypeStr);

        if (fieldType == FIELDTYPE_VARCHAR)
        {
            int colLength = row[5].toInt(); // 获取列长度
            QString escapedValue = values[valueIndex].trimmed();
            if (escapedValue.isEmpty())
            {
                columnValues.append("NULL");
            }
            else
            {
                columnValues.append(QString("'%1'").arg(escapedValue.replace("'", "''")));
            }
        }
        else if (fieldType == FIELDTYPE_INTEGER)
        {
            columnValues.append(values[valueIndex].isEmpty() ? "NULL" : values[valueIndex]);
        }
        else if (fieldType == FIELDTYPE_REAL)
        {
            columnValues.append(values[valueIndex].isEmpty() ? "NULL" : values[valueIndex]);
        }
        valueIndex++;
    }

    sql += columnNames.join(", ") + ") VALUES (" + columnValues.join(", ") + ")";
    sql += ";";

    // 添加调试信息
    qDebug() << "执行插入语句:" << sql;

    rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);

    if (rc != 0)
    {
        QString errorMsg = QString("插入数据失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        showError(errorMsg);
        if (errmsg)
            free(errmsg);
        return;
    }

    // 刷新表格数据
    onTableSelected(tableTree->currentItem(), 0);
}

void MainWindow::executeUpdateSQL(const QString &tableName, const QStringList &values, int rowIndex, const QMap<int, FieldType> &columnFieldTypes)
{
    if (!db)
        return;

    // 首先获取表结构信息
    SQLResult schemaResult;
    QString schemaSql = QString("SELECT * FROM schema WHERE tableName = '%1' ORDER BY columnIndex").arg(tableName);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, schemaSql.toUtf8().constData(), sqlResultCallback, &schemaResult, &errmsg);

    if (rc != 0)
    {
        QString errorMsg = QString("获取表结构失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        showError(errorMsg);
        if (errmsg)
            free(errmsg);
        return;
    }

    // 构建更新语句
    QStringList updateParts;
    for (int i = 0; i < values.size(); ++i)
    {
        QString columnName = schemaResult.rows[i][1]; // columnName
        QString value = values[i];

        // 根据数据类型决定是否添加引号
        if (columnFieldTypes.contains(i) &&
            (columnFieldTypes[i] == FieldType::FIELDTYPE_VARCHAR))
        {
            // 字符串类型添加引号
            updateParts << QString("%1 = '%2'").arg(columnName).arg(value);
        }
        else
        {
            // 数值类型不添加引号
            updateParts << QString("%1 = %2").arg(columnName).arg(value);
        }
    }

    QString sql = QString("UPDATE %1 SET %2 WHERE rowid = %3")
                      .arg(tableName)
                      .arg(updateParts.join(", "))
                      .arg(rowIndex + 1); // SQLite 的 rowid 从 1 开始

    // 调试输出 SQL 语句
    qDebug() << "执行更新 SQL:" << sql;

    rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
    if (rc != 0)
    {
        QString errorMsg = QString("更新数据失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        showError(errorMsg);
        if (errmsg)
            free(errmsg);
    }
}

void MainWindow::executeDeleteSQL(const QString &tableName, int rowIndex)
{
    qDebug() << "开始执行删除操作，表名:" << tableName << "，行索引:" << rowIndex;

    // 获取主键列名和类型
    SQLResult result;
    QString sql = QString("SELECT * FROM schema WHERE tableName = '%1'").arg(tableName);
    char *errmsg = nullptr;
    qDebug() << "执行查询schema语句:" << sql;

    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    qDebug() << "查询schema返回码:" << rc;

    if (errmsg)
    {
        qDebug() << "错误信息:" << errmsg;
        free(errmsg);
    }

    if (rc != 0)
    {
        QString errorMsg = QString("获取表结构失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        qDebug() << errorMsg;
        showError(errorMsg);
        return;
    }

    qDebug() << "获取到的表结构:";
    for (const QStringList &row : result.rows)
    {
        qDebug() << row;
    }

    // 查找主键列
    QString primaryKeyColumn;
    int primaryKeyIndex = -1;
    int primaryKeyType = -1;
    for (int i = 0; i < result.rows.size(); i++)
    {
        const QStringList &row = result.rows[i];
        if (row[8] == "1")
        {
            primaryKeyColumn = row[1];
            primaryKeyIndex = i;
            primaryKeyType = row[4].toInt();
            break;
        }
    }

    if (primaryKeyColumn.isEmpty())
    {
        qDebug() << "错误：未找到主键列";
        showError("未找到主键列");
        return;
    }
    qDebug() << "找到主键列:" << primaryKeyColumn << "，类型:" << primaryKeyType << "，索引:" << primaryKeyIndex;

    // 获取主键值
    QString primaryKeyValue;
    if (dataTable->item(rowIndex, primaryKeyIndex))
    {
        primaryKeyValue = dataTable->item(rowIndex, primaryKeyIndex)->text();
        qDebug() << "获取到的主键值:" << primaryKeyValue;
    }
    else
    {
        qDebug() << "错误：无法获取主键值，行索引可能无效";
        showError("无法获取要删除的记录");
        return;
    }

    // 根据主键类型构建删除语句
    if (primaryKeyType == FIELDTYPE_INTEGER || primaryKeyType == FIELDTYPE_REAL)
    {
        // 数值类型不加引号
        sql = QString("DELETE FROM %1 WHERE %2 = %3")
                  .arg(tableName)
                  .arg(primaryKeyColumn)
                  .arg(primaryKeyValue);
    }
    else
    {
        // 字符串类型使用双引号
        sql = QString("DELETE FROM %1 WHERE %2 = \"%3\"")
                  .arg(tableName)
                  .arg(primaryKeyColumn)
                  .arg(primaryKeyValue);
    }

    qDebug() << "执行删除SQL:" << sql;

    rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
    qDebug() << "删除操作返回码:" << rc;

    if (errmsg)
    {
        qDebug() << "错误信息:" << errmsg;
        free(errmsg);
    }

    if (rc != 0)
    {
        QString errorMsg = QString("删除数据失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
        qDebug() << errorMsg;
        showError(errorMsg);
        return;
    }

    qDebug() << "删除操作成功，刷新表格数据";
    // 刷新表格数据
    onTableSelected(tableTree->currentItem(), 0);
}

void MainWindow::onFileAction()
{
    // 文件操作的实现
    qDebug() << "文件操作被触发";
}

void MainWindow::onViewAction()
{
    QMenu viewMenu(this);

    // 添加树形图选项
    QAction *treeAction = viewMenu.addAction("树形图");
    treeAction->setCheckable(true);
    
    // 获取当前左侧面板的可见状态
    bool isTreeVisible = true;
    if (mainSplitter) {
        QList<int> sizes = mainSplitter->sizes();
        isTreeVisible = sizes[0] > 0;
    }
    treeAction->setChecked(isTreeVisible);

    // 添加分隔线
    viewMenu.addSeparator();

    // 添加标签页选项，带复选框
    QAction *dbTabAction = viewMenu.addAction("数据库标签");
    QAction *dataTabAction = viewMenu.addAction("数据标签");
    QAction *ddlTabAction = viewMenu.addAction("DDL标签");
    QAction *designTabAction = viewMenu.addAction("设计标签");
    QAction *sqlTabAction = viewMenu.addAction("SQL标签");

    // 创建自定义复选框图标
    QPixmap checkedPixmap(16, 16);
    checkedPixmap.fill(Qt::transparent);
    QPainter checkedPainter(&checkedPixmap);
    checkedPainter.setPen(QPen(Qt::black, 2));
    checkedPainter.drawRect(2, 2, 12, 12);
    checkedPainter.drawLine(4, 8, 7, 11);
    checkedPainter.drawLine(7, 11, 12, 4);
    QIcon checkedIcon(checkedPixmap);

    QPixmap uncheckedPixmap(16, 16);
    uncheckedPixmap.fill(Qt::transparent);
    QPainter uncheckedPainter(&uncheckedPixmap);
    uncheckedPainter.setPen(QPen(Qt::black, 2));
    uncheckedPainter.drawRect(2, 2, 12, 12);
    QIcon uncheckedIcon(uncheckedPixmap);

    // 设置复选框
    dbTabAction->setCheckable(true);
    dataTabAction->setCheckable(true);
    ddlTabAction->setCheckable(true);
    designTabAction->setCheckable(true);
    sqlTabAction->setCheckable(true);

    // 根据当前标签页的可见性设置复选框状态和图标
    dbTabAction->setChecked(rightTabWidget->isTabVisible(0));
    dataTabAction->setChecked(rightTabWidget->isTabVisible(1));
    ddlTabAction->setChecked(rightTabWidget->isTabVisible(2));
    designTabAction->setChecked(rightTabWidget->isTabVisible(3));
    sqlTabAction->setChecked(rightTabWidget->isTabVisible(4));
    
    treeAction->setIcon(isTreeVisible ? checkedIcon : uncheckedIcon);
    dbTabAction->setIcon(dbTabAction->isChecked() ? checkedIcon : uncheckedIcon);
    dataTabAction->setIcon(dataTabAction->isChecked() ? checkedIcon : uncheckedIcon);
    ddlTabAction->setIcon(ddlTabAction->isChecked() ? checkedIcon : uncheckedIcon);
    designTabAction->setIcon(designTabAction->isChecked() ? checkedIcon : uncheckedIcon);
    sqlTabAction->setIcon(sqlTabAction->isChecked() ? checkedIcon : uncheckedIcon);

    // 显示菜单
    QAction *selectedAction = viewMenu.exec(QCursor::pos());

    if (selectedAction)
    {
        if (selectedAction == treeAction)
        {
            // 处理树形图的显示/隐藏
            if (mainSplitter) {
                QList<int> sizes = mainSplitter->sizes();
                if (!treeAction->isChecked()) {
                    // 保存当前大小比例，以便恢复时使用
                    mainSplitter->setProperty("savedTreeSize", sizes[0]);
                    sizes[0] = 0;
                } else {
                    // 恢复之前保存的大小，或使用默认值
                    int savedSize = mainSplitter->property("savedTreeSize").toInt();
                    sizes[0] = savedSize > 0 ? savedSize : 300;
                }
                mainSplitter->setSizes(sizes);
                treeAction->setIcon(treeAction->isChecked() ? checkedIcon : uncheckedIcon);
            }
        }
        else if (selectedAction == dbTabAction)
        {
            rightTabWidget->setTabVisible(0, dbTabAction->isChecked());
            dbTabAction->setIcon(dbTabAction->isChecked() ? checkedIcon : uncheckedIcon);
        }
        else if (selectedAction == dataTabAction)
        {
            rightTabWidget->setTabVisible(1, dataTabAction->isChecked());
            dataTabAction->setIcon(dataTabAction->isChecked() ? checkedIcon : uncheckedIcon);
        }
        else if (selectedAction == ddlTabAction)
        {
            rightTabWidget->setTabVisible(2, ddlTabAction->isChecked());
            ddlTabAction->setIcon(ddlTabAction->isChecked() ? checkedIcon : uncheckedIcon);
        }
        else if (selectedAction == designTabAction)
        {
            rightTabWidget->setTabVisible(3, designTabAction->isChecked());
            designTabAction->setIcon(designTabAction->isChecked() ? checkedIcon : uncheckedIcon);
        }
        else if (selectedAction == sqlTabAction)
        {
            rightTabWidget->setTabVisible(4, sqlTabAction->isChecked());
            sqlTabAction->setIcon(sqlTabAction->isChecked() ? checkedIcon : uncheckedIcon);
        }

        // 如果当前显示的标签页被隐藏，则切换到第一个可见的标签页
        if (!rightTabWidget->isTabVisible(rightTabWidget->currentIndex()))
        {
            for (int i = 0; i < rightTabWidget->count(); ++i)
            {
                if (rightTabWidget->isTabVisible(i))
                {
                    rightTabWidget->setCurrentIndex(i);
                    break;
                }
            }
        }
    }
}

void MainWindow::onDatabaseAction()
{
    // 数据库操作的实现
    qDebug() << "数据库操作被触发";
}

void MainWindow::onSQLAction()
{
    // SQL操作的实现
    qDebug() << "SQL操作被触发";
}

void MainWindow::onTransactionAction()
{
    // 事务操作的实现
    qDebug() << "事务操作被触发";
}

void MainWindow::onToolsAction()
{
    // 工具操作的实现
    qDebug() << "工具操作被触发";
}

void MainWindow::onHelpAction()
{
    // 帮助操作的实现
    qDebug() << "帮助操作被触发";
}

void MainWindow::onObjectAction()
{
    // 对象操作的实现
    qDebug() << "对象操作被触发";
}

void MainWindow::onTableManagementAction()
{
    // 表管理操作的实现
    qDebug() << "表管理操作被触发";
}

void MainWindow::onNewDatabase()
{
    if (db)
    {
        QMessageBox::warning(this, "警告", "已经连接到数据库，请先断开连接");
        return;
    }

    // 打开文件保存对话框
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("新建数据库文件"),
                                                    QDir::currentPath(),
                                                    tr("数据库文件 (*.db *.dat)"));

    if (fileName.isEmpty())
    {
        return;
    }

    // 确保文件扩展名正确
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix().toLower();
    if (suffix != "db" && suffix != "dat")
    {
        fileName += ".db"; // 默认使用.db扩展名
    }

    // 获取数据库文件所在目录
    QString dbDir = QFileInfo(fileName).absolutePath();

    // 检查log目录是否存在
    QDir dir(dbDir);
    if (!dir.exists("log"))
    {
        // 创建log目录
        if (!dir.mkdir("log"))
        {
            QMessageBox::warning(this, "警告", "无法创建log目录");
            return;
        }
        qDebug() << "已创建log目录:" << dbDir + "/log";
    }

    // 创建新数据库
    db = new GNCDB();
    int rc;

    qDebug() << "尝试创建新数据库文件：" << fileName;

    // 将 QString 转换为 char* 类型
    QByteArray dbPathBytes = fileName.toLocal8Bit();
    char *dbPathChar = dbPathBytes.data();

    if ((rc = GNCDB_open(&db, dbPathChar)) == 0)
    {
        statusBar->showMessage("数据库创建成功");
        dbNameLabel->setText(QString("数据库: %1").arg(QFileInfo(fileName).baseName()));
        dbPathLabel->setText(QString("路径: %1").arg(fileName));
        updateTableList();
    }
    else
    {
        QString errorMsg = QString("数据库创建失败，错误代码: %1，文件路径: %2").arg(rc).arg(fileName);
        showError(errorMsg);
        delete db;
        db = nullptr;
    }
}

void MainWindow::onExit()
{
    // 如果已连接数据库，先断开连接
    if (db)
    {
        onDisconnectDB();
    }

    // 退出应用程序
    QApplication::quit();
}

// 添加closeEvent函数来处理窗口关闭事件
void MainWindow::closeEvent(QCloseEvent *event)
{
    // 如果已连接数据库，先断开连接
    if (db)
    {
        onDisconnectDB();
    }

    // 接受关闭事件
    saveWindowSettings();
    QMainWindow::closeEvent(event);
    event->accept();
}

void MainWindow::onShowTreeView()
{
    // 设置焦点到树形图
    tableTree->setFocus();

    // 恢复之前选中的项
    QTreeWidgetItem *currentItem = tableTree->currentItem();
    if (currentItem)
    {
        currentItem->setSelected(true);
    }
}

void MainWindow::onShowDatabaseTab()
{
    if (rightTabWidget)
    {
        rightTabWidget->setCurrentIndex(0);
    }
}

void MainWindow::onShowDataTab()
{
    if (rightTabWidget)
    {
        rightTabWidget->setCurrentIndex(1);
    }
}

void MainWindow::onShowDDLTab()
{
    if (rightTabWidget)
    {
        rightTabWidget->setCurrentIndex(2);
    }
}

void MainWindow::onShowDesignTab()
{
    if (rightTabWidget)
    {
        rightTabWidget->setCurrentIndex(3);
    }
}

void MainWindow::onShowSQLTab()
{
    if (rightTabWidget)
    {
        rightTabWidget->setCurrentIndex(4);
    }
}

void MainWindow::updateDDLView(const QString &tableName)
{
    qDebug() << "更新DDL视图，表名:" << tableName;

    if (!db || tableName.isEmpty())
    {
        qDebug() << "数据库未连接或表名为空";
        return;
    }

    if (!ddlEditor)
    {
        qDebug() << "DDL编辑器未初始化";
        return;
    }

    // 查询master表获取表的创建语句
    SQLResult result;
    QString sql = QString("SELECT sql FROM master WHERE tableName = '%1'").arg(tableName);
    qDebug() << "执行SQL:" << sql;

    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);

    if (rc != 0)
    {
        QString errorMsg = QString("获取表DDL失败: %1").arg(rc);
        qDebug() << errorMsg;
        showError(errorMsg);
        if (errmsg)
        {
            qDebug() << "错误信息:" << errmsg;
            free(errmsg);
        }
        return;
    }

    qDebug() << "查询结果行数:" << result.rows.size();

    // 显示DDL语句
    if (!result.rows.isEmpty())
    {
        QString ddl = result.rows[0][0];
        qDebug() << "DDL语句:" << ddl;
        ddlEditor->setText(ddl);
    }
    else
    {
        qDebug() << "未找到表的创建语句";
        ddlEditor->setText("未找到表的创建语句");
    }
}

// void MainWindow::onRenameTable()
// {
//     if (!db) {
//         showError("请先连接数据库");
//         return;
//     }

//     // 获取当前选中的表
//     QTreeWidgetItem *currentItem = tableTree->currentItem();
//     if (!currentItem) {
//         showError("请先选择要重命名的表");
//         return;
//     }

//     QString oldTableName = currentItem->text(0);

//     // 创建重命名对话框
//     QDialog dialog(this);
//     dialog.setWindowTitle("重命名表");
//     QVBoxLayout *layout = new QVBoxLayout(&dialog);

//     // 新表名输入
//     QHBoxLayout *nameLayout = new QHBoxLayout();
//     QLabel *nameLabel = new QLabel("新表名:", &dialog);
//     QLineEdit *nameEdit = new QLineEdit(&dialog);
//     nameEdit->setText(oldTableName);
//     nameLayout->addWidget(nameLabel);
//     nameLayout->addWidget(nameEdit);
//     layout->addLayout(nameLayout);

//     // 确定和取消按钮
//     QDialogButtonBox *buttonBox = new QDialogButtonBox(
//         QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
//         Qt::Horizontal, &dialog);
//     layout->addWidget(buttonBox);

//     // 连接确定按钮
//     connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
//         QString newTableName = nameEdit->text().trimmed();
//         if (newTableName.isEmpty()) {
//             showError("请输入新表名");
//             return;
//         }

//         if (newTableName == oldTableName) {
//             dialog.reject();
//             return;
//         }

//         // 构建重命名表的SQL语句
//         QString sql = QString("ALTER TABLE %1 RENAME TO %2").arg(oldTableName).arg(newTableName);

//         // 执行重命名操作
//         char *errmsg = nullptr;
//         int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);

//         if (rc != 0) {
//             QString errorMsg = QString("重命名表失败: %1").arg(rc);
//             showError(errorMsg);
//             if (errmsg) free(errmsg);
//             return;
//         }

//         // 刷新表列表
//         updateTableList();
//         dialog.accept();
//     });

//     // 连接取消按钮
//     connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

//     // 显示对话框
//     dialog.exec();
// }

void MainWindow::onClearTable()
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }

    // 获取当前选中的表
    QTreeWidgetItem *currentItem = tableTree->currentItem();
    if (!currentItem)
    {
        showError("请先选择要清空的表");
        return;
    }

    QString tableName = currentItem->text(0);

    // 确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认清空",
        QString("确定要清空表 %1 吗？此操作不可恢复！").arg(tableName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        // 构建清空表的SQL语句
        QString sql = QString("DELETE FROM %1").arg(tableName);

        // 执行清空操作
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);

        if (rc != 0)
        {
            QString errorMsg = QString("清空表失败: %1").arg(Rc2Msg::getErrorMsg(rc, errmsg ? QString(errmsg) : QString()));
            showError(errorMsg);
            if (errmsg)
                free(errmsg);
            return;
        }

        // 刷新表格数据
        onTableSelected(currentItem, 0);
    }
}

void MainWindow::onZoomIn()
{
    QFont currentFont = font();
    currentFont.setPointSize(currentFont.pointSize() + 1);
    setFont(currentFont);

    // 更新所有子部件的字体
    QList<QWidget *> widgets = findChildren<QWidget *>();
    for (QWidget *widget : widgets)
    {
        widget->setFont(currentFont);
    }
}

void MainWindow::onZoomOut()
{
    QFont currentFont = font();
    if (currentFont.pointSize() > 1)
    { // 确保字体大小不会小于1
        currentFont.setPointSize(currentFont.pointSize() - 1);
        setFont(currentFont);

        // 更新所有子部件的字体
        QList<QWidget *> widgets = findChildren<QWidget *>();
        for (QWidget *widget : widgets)
        {
            widget->setFont(currentFont);
        }
    }
}

void MainWindow::onExecuteCurrentLine()
{
    if (!sqlEditor)
        return;

    // 获取当前光标位置
    QTextCursor cursor = sqlEditor->textCursor();
    int currentLine = cursor.blockNumber();

    // 获取当前行的文本
    QString currentLineText = sqlEditor->document()->findBlockByLineNumber(currentLine).text().trimmed();

    if (currentLineText.isEmpty())
    {
        showError("当前行为空，无法执行");
        return;
    }

    // 执行当前行的SQL
    executeSQLStatement(currentLineText);
}

void MainWindow::onFormatSQL()
{
    if (!sqlEditor)
        return;

    QString sql = sqlEditor->toPlainText();
    if (sql.isEmpty())
    {
        showError("SQL语句为空，无法格式化");
        return;
    }

    // 简单的SQL格式化规则
    // 1. 将关键字转换为大写
    QStringList keywords = {"SELECT", "FROM", "WHERE", "GROUP BY", "ORDER BY", "HAVING",
                            "INSERT", "INTO", "VALUES", "UPDATE", "SET", "DELETE",
                            "CREATE", "TABLE", "ALTER", "DROP", "INDEX", "VIEW"};

    for (const QString &keyword : keywords)
    {
        QRegularExpression regex(QString("\\b%1\\b").arg(keyword), QRegularExpression::CaseInsensitiveOption);
        sql.replace(regex, keyword);
    }

    // 2. 在关键字前添加换行
    for (const QString &keyword : keywords)
    {
        if (keyword != "SELECT" && keyword != "INSERT" && keyword != "UPDATE" && keyword != "DELETE")
        {
            QRegularExpression regex(QString("\\b%1\\b").arg(keyword), QRegularExpression::CaseInsensitiveOption);
            sql.replace(regex, QString("\n%1").arg(keyword));
        }
    }

    // 3. 在逗号后添加空格
    sql.replace(QRegularExpression(",\\s*"), ", ");

    // 4. 在括号前后添加空格
    sql.replace(QRegularExpression("\\(\\s*"), "( ");
    sql.replace(QRegularExpression("\\s*\\)"), " )");

    // 更新编辑器内容
    sqlEditor->setPlainText(sql);
}

void MainWindow::onOpenSQLScript()
{
    if (!sqlEditor)
        return;

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("打开SQL脚本"),
                                                    QDir::currentPath(),
                                                    tr("SQL文件 (*.sql);;所有文件 (*.*)"));

    if (fileName.isEmpty())
    {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        showError("无法打开文件");
        return;
    }

    // 使用QTextStream读取文件，Qt 6默认使用UTF-8编码
    QTextStream in(&file);
    sqlEditor->setPlainText(in.readAll());
    file.close();
}

void MainWindow::onSaveSQLScript()
{
    if (!sqlEditor)
        return;

    // 如果当前没有打开的文件，则调用另存为
    if (currentSQLFile.isEmpty())
    {
        onSaveAsSQLScript();
        return;
    }

    QFile file(currentSQLFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        showError("无法保存文件");
        return;
    }

    // 使用QTextStream写入文件，Qt 6默认使用UTF-8编码
    QTextStream out(&file);
    out << sqlEditor->toPlainText();
    file.close();
}

void MainWindow::onSaveAsSQLScript()
{
    if (!sqlEditor)
        return;

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("保存SQL脚本"),
                                                    QDir::currentPath(),
                                                    tr("SQL文件 (*.sql);;所有文件 (*.*)"));

    if (fileName.isEmpty())
    {
        return;
    }

    // 确保文件有.sql扩展名
    if (!fileName.endsWith(".sql", Qt::CaseInsensitive))
    {
        fileName += ".sql";
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        showError("无法保存文件");
        return;
    }

    // 使用QTextStream写入文件，Qt 6默认使用UTF-8编码
    QTextStream out(&file);
    out << sqlEditor->toPlainText();
    file.close();

    // 更新当前文件路径
    currentSQLFile = fileName;
}

// 辅助函数：执行SQL语句
void MainWindow::executeSQLStatement(const QString &sql)
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }

    if (sql.isEmpty())
    {
        showError("SQL语句为空");
        return;
    }

    // 创建SQL结果对象
    SQLResult result;

    // 执行SQL语句
    char *errmsg = nullptr;
    qDebug() << "调用GNCDB_exec...";
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    qDebug() << "GNCDB_exec返回码:" << rc;

    if (rc != 0)
    {
        QString errorMsg = QString("SQL执行失败: %1").arg(Rc2Msg::getErrorMsg(rc));
        qDebug() << "错误代码:" << rc;
        showError(errorMsg);
        if (errmsg)
        {
            free(errmsg);
        }
        return;
    }

    // 清空并设置结果表格
    sqlResultDisplay->clear();
    sqlResultDisplay->setRowCount(0);
    sqlResultDisplay->setColumnCount(result.columnNames.size());
    sqlResultDisplay->setHorizontalHeaderLabels(result.columnNames);

    // 设置表格属性
    sqlResultDisplay->setEditTriggers(QTableWidget::NoEditTriggers);
    sqlResultDisplay->setSelectionBehavior(QTableWidget::SelectItems);
    sqlResultDisplay->setSelectionMode(QTableWidget::SingleSelection);
    sqlResultDisplay->setAlternatingRowColors(true);

    // 设置自定义委托
    SQLTableDelegate *delegate = new SQLTableDelegate(sqlResultDisplay);
    sqlResultDisplay->setItemDelegate(delegate);

    // 连接数据改变信号
    connect(delegate, &SQLTableDelegate::dataChanged, this, [this](const QModelIndex &index, const QString &newValue, const QString &oldValue)
            {
        qDebug() << "=== 单元格内容改变 ===";
        qDebug() << "行:" << index.row() << "列:" << index.column();
        qDebug() << "新值:" << newValue;
        qDebug() << "旧值:" << oldValue;
        
        // 获取列名和主键信息
        QString columnName = sqlResultDisplay->horizontalHeaderItem(index.column())->text();
        QString primaryKeyValue = sqlResultDisplay->item(index.row(), 0)->text();
        
        qDebug() << "列名:" << columnName;
        qDebug() << "主键值:" << primaryKeyValue;
        
        // 构建更新语句
        QString updateSql = QString("UPDATE %1 SET %2 = '%3' WHERE rowid = %4")
                             .arg(currentTable)
                             .arg(columnName)
                             .arg(newValue)
                             .arg(primaryKeyValue);
                             
        qDebug() << "执行SQL:" << updateSql;
        
        // 执行更新
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, updateSql.toUtf8().constData(), nullptr, nullptr, &errmsg);
        
        if (rc != 0) {
            qDebug() << "更新失败，错误码:" << rc;
            qDebug() << "错误信息:" << (errmsg ? errmsg : "未知错误");
            // 恢复原值
            sqlResultDisplay->item(index.row(), index.column())->setText(oldValue);
            if (errmsg) free(errmsg);
        } else {
            qDebug() << "更新成功";
        }
        qDebug() << "=== 处理完成 ==="; });

    // 设置行数据
    sqlResultDisplay->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); row++)
    {
        const QStringList &rowData = result.rows[row];
        for (int col = 0; col < rowData.size(); col++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[col]);
            // 保存原始值到UserRole
            item->setData(Qt::UserRole, rowData[col]);
            // 设置标志位，允许编辑
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            sqlResultDisplay->setItem(row, col, item);
        }
    }

    // 自动调整列宽
    sqlResultDisplay->resizeColumnsToContents();

    // 更新状态栏
    QString statusMsg = QString("查询完成，返回 %1 行数据").arg(result.rows.size());
    statusBar->showMessage(statusMsg);
}

void MainWindow::onCellEdited(QTableWidgetItem *item)
{
    qDebug() << "=== 开始处理单元格编辑 ===";
    qDebug() << "当前表名:" << currentTable;
    qDebug() << "数据库状态:" << (db ? "已连接" : "未连接");
    qDebug() << "Item指针:" << item;

    if (!db || !item)
    {
        qDebug() << "数据库未连接或item为空";
        return;
    }

    QString newValue = item->text();
    QString oldValue = item->data(Qt::UserRole).toString();

    qDebug() << "单元格编辑详情:";
    qDebug() << "  行:" << item->row();
    qDebug() << "  列:" << item->column();
    qDebug() << "  新值:" << newValue;
    qDebug() << "  旧值:" << oldValue;

    if (newValue == oldValue)
    {
        qDebug() << "值未改变，不需要更新";
        return;
    }

    // 获取列名和主键信息
    QString columnName = sqlResultDisplay->horizontalHeaderItem(item->column())->text();
    QString primaryKeyValue = sqlResultDisplay->item(item->row(), 0)->text();

    qDebug() << "更新信息:";
    qDebug() << "  列名:" << columnName;
    qDebug() << "  主键值:" << primaryKeyValue;

    // 构建更新语句
    QString sql = QString("UPDATE %1 SET %2 = %3 WHERE rowid = %4")
                      .arg(currentTable)
                      .arg(columnName)
                      .arg(newValue)
                      .arg(primaryKeyValue);

    qDebug() << "执行SQL:" << sql;

    // 执行更新
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);

    if (rc != 0)
    {
        qDebug() << "更新失败，错误码:" << rc;
        qDebug() << "错误信息:" << (errmsg ? errmsg : "未知错误");
        // 恢复原值
        item->setText(oldValue);
        if (errmsg)
            free(errmsg);
    }
    else
    {
        qDebug() << "更新成功";
        // 更新UserRole数据
        item->setData(Qt::UserRole, newValue);
    }
    qDebug() << "=== 单元格编辑处理完成 ===";
}

void MainWindow::onSearchTable()
{
    if (!db)
    {
        showError("请先连接数据库");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("表查找");
    dialog.setMinimumWidth(400);
    dialog.setMinimumHeight(300);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 创建搜索框
    QLineEdit *searchEdit = new QLineEdit(&dialog);
    searchEdit->setPlaceholderText(tr("输入表名进行搜索..."));
    searchEdit->setStyleSheet(
        "QLineEdit {"
        "    padding: 5px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    background: white;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #0078d7;"
        "}");
    layout->addWidget(searchEdit);

    // 创建表格
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(1);
    table->setHorizontalHeaderLabels(QStringList() << tr("表名"));
    table->setSelectionBehavior(QTableWidget::SelectRows);
    table->setSelectionMode(QTableWidget::SingleSelection);
    table->setEditTriggers(QTableWidget::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    background: white;"
        "    gridline-color: #f0f0f0;"
        "}"
        "QTableWidget::item {"
        "    padding: 5px;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #0078d7;"
        "    color: white;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f5f5f5;"
        "    padding: 5px;"
        "    border: none;"
        "    border-bottom: 1px solid #ccc;"
        "    font-weight: bold;"
        "}");
    layout->addWidget(table);

    // 添加按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *searchButton = new QPushButton(tr("搜索"), &dialog);
    QPushButton *closeButton = new QPushButton(tr("关闭"), &dialog);
    searchButton->setStyleSheet(
        "QPushButton {"
        "    padding: 5px 15px;"
        "    background-color: #0078d7;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #005a9e;"
        "}");
    closeButton->setStyleSheet(
        "QPushButton {"
        "    padding: 5px 15px;"
        "    background-color: #f0f0f0;"
        "    color: #333;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;"
        "}");
    buttonLayout->addStretch();
    buttonLayout->addWidget(searchButton);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    // 连接信号
    connect(searchButton, &QPushButton::clicked, [&]()
            {
        QString searchText = searchEdit->text().trimmed();
        if (searchText.isEmpty()) {
            showError("请输入要查找的表名");
            return;
        }

        // 清空结果列表
        table->setRowCount(0);

        // 在表树中查找
        QList<QTreeWidgetItem*> items = tableTree->findItems(searchText, Qt::MatchContains | Qt::MatchRecursive);
        if (items.isEmpty()) {
            showError(QString("未找到包含 '%1' 的表").arg(searchText));
            return;
        }

        // 添加结果到列表
        for (QTreeWidgetItem *item : items) {
            int newRow = table->rowCount();
            table->insertRow(newRow);
            QTableWidgetItem *tableItem = new QTableWidgetItem(item->text(0));
            table->setItem(newRow, 0, tableItem);
        }

        // 调整列宽
        table->resizeColumnsToContents();
        
        // 连接结果列表的点击事件
        connect(table, &QTableWidget::itemClicked, [&](QTableWidgetItem *item) {
            QString tableName = item->text();
            QList<QTreeWidgetItem*> foundItems = tableTree->findItems(tableName, Qt::MatchExactly);
            if (!foundItems.isEmpty()) {
                tableTree->setCurrentItem(foundItems.first());
                tableTree->scrollToItem(foundItems.first());
            }
        }); });

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::onSearchColumn()
{
    if (!db || currentTable.isEmpty())
    {
        showError("请先连接数据库并选择表");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("字段查找");
    dialog.setMinimumWidth(400);
    dialog.setMinimumHeight(300);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 创建搜索框
    QLineEdit *searchEdit = new QLineEdit(&dialog);
    searchEdit->setPlaceholderText(tr("输入字段名进行搜索..."));
    searchEdit->setStyleSheet(
        "QLineEdit {"
        "    padding: 5px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    background: white;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #0078d7;"
        "}");
    layout->addWidget(searchEdit);

    // 创建表格
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("列号") << tr("列名"));
    table->setSelectionBehavior(QTableWidget::SelectRows);
    table->setSelectionMode(QTableWidget::SingleSelection);
    table->setEditTriggers(QTableWidget::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    background: white;"
        "    gridline-color: #f0f0f0;"
        "}"
        "QTableWidget::item {"
        "    padding: 5px;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #0078d7;"
        "    color: white;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f5f5f5;"
        "    padding: 5px;"
        "    border: none;"
        "    border-bottom: 1px solid #ccc;"
        "    font-weight: bold;"
        "}");
    layout->addWidget(table);

    // 添加按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *searchButton = new QPushButton(tr("搜索"), &dialog);
    QPushButton *closeButton = new QPushButton(tr("关闭"), &dialog);
    searchButton->setStyleSheet(
        "QPushButton {"
        "    padding: 5px 15px;"
        "    background-color: #0078d7;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #005a9e;"
        "}");
    closeButton->setStyleSheet(
        "QPushButton {"
        "    padding: 5px 15px;"
        "    background-color: #f0f0f0;"
        "    color: #333;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;"
        "}");
    buttonLayout->addStretch();
    buttonLayout->addWidget(searchButton);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    // 连接信号
    connect(searchButton, &QPushButton::clicked, [&]()
            {
        QString searchText = searchEdit->text().trimmed();
        if (searchText.isEmpty()) {
            showError("请输入要查找的段名");
            return;
        }

        // 清空结果列表
        table->setRowCount(0);

        // 在表头中查找
        for (int i = 0; i < dataTable->columnCount(); ++i) {
            QString headerText = dataTable->horizontalHeaderItem(i)->text();
            if (headerText.contains(searchText, Qt::CaseInsensitive)) {
                int newRow = table->rowCount();
                table->insertRow(newRow);
                
                // 添加列号
                QTableWidgetItem *colItem = new QTableWidgetItem(QString::number(i + 1));
                table->setItem(newRow, 0, colItem);
                
                // 添加列名
                QTableWidgetItem *nameItem = new QTableWidgetItem(headerText);
                table->setItem(newRow, 1, nameItem);
            }
        }

        if (table->rowCount() == 0) {
            showError(QString("未找到包含 '%1' 的段").arg(searchText));
            return;
        }

        // 调整列宽
        table->resizeColumnsToContents();
        
        // 连接结果列表的点击事件
        connect(table, &QTableWidget::itemClicked, [&](QTableWidgetItem *item) {
            int col = table->item(item->row(), 0)->text().toInt() - 1;
            dataTable->selectColumn(col);
            dataTable->scrollToItem(dataTable->item(0, col));
        }); });

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::onSearchValue()
{
    if (!db || currentTable.isEmpty())
    {
        showError("请先连接数据库并选择表");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("数值查找");
    dialog.setMinimumWidth(400);
    dialog.setMinimumHeight(300);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 创建搜索框
    QLineEdit *searchEdit = new QLineEdit(&dialog);
    searchEdit->setPlaceholderText(tr("输入要查找的值..."));
    searchEdit->setStyleSheet(
        "QLineEdit {"
        "    padding: 5px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    background: white;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #0078d7;"
        "}");
    layout->addWidget(searchEdit);

    // 创建表格
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(QStringList() << tr("行号") << tr("列名") << tr("值"));
    table->setSelectionBehavior(QTableWidget::SelectRows);
    table->setSelectionMode(QTableWidget::SingleSelection);
    table->setEditTriggers(QTableWidget::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setStyleSheet(
        "QTableWidget {"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    background: white;"
        "    gridline-color: #f0f0f0;"
        "}"
        "QTableWidget::item {"
        "    padding: 5px;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #0078d7;"
        "    color: white;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f5f5f5;"
        "    padding: 5px;"
        "    border: none;"
        "    border-bottom: 1px solid #ccc;"
        "    font-weight: bold;"
        "}");
    layout->addWidget(table);

    // 添加按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *searchButton = new QPushButton(tr("搜索"), &dialog);
    QPushButton *closeButton = new QPushButton(tr("关闭"), &dialog);
    searchButton->setStyleSheet(
        "QPushButton {"
        "    padding: 5px 15px;"
        "    background-color: #0078d7;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #005a9e;"
        "}");
    closeButton->setStyleSheet(
        "QPushButton {"
        "    padding: 5px 15px;"
        "    background-color: #f0f0f0;"
        "    color: #333;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;"
        "}");
    buttonLayout->addStretch();
    buttonLayout->addWidget(searchButton);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    // 连接信号
    connect(searchButton, &QPushButton::clicked, [&]()
            {
        QString searchText = searchEdit->text().trimmed();
        if (searchText.isEmpty()) {
            showError("请输入要查找的数值");
            return;
        }

        // 清空结果列表
        table->setRowCount(0);

        // 在表格中查找
        for (int row = 0; row < dataTable->rowCount(); ++row) {
            for (int col = 0; col < dataTable->columnCount(); ++col) {
                QTableWidgetItem *item = dataTable->item(row, col);
                if (item && item->text().contains(searchText, Qt::CaseInsensitive)) {
                    int newRow = table->rowCount();
                    table->insertRow(newRow);
                    
                    // 添加行号
                    QTableWidgetItem *rowItem = new QTableWidgetItem(QString::number(row + 1));
                    table->setItem(newRow, 0, rowItem);
                    
                    // 添加列名
                    QTableWidgetItem *colItem = new QTableWidgetItem(dataTable->horizontalHeaderItem(col)->text());
                    table->setItem(newRow, 1, colItem);
                    
                    // 添加值
                    QTableWidgetItem *valueItem = new QTableWidgetItem(item->text());
                    table->setItem(newRow, 2, valueItem);
                }
            }
        }

        if (table->rowCount() == 0) {
            showError(QString("未找到包含 '%1' 的数值").arg(searchText));
            return;
        }

        // 调整列宽
        table->resizeColumnsToContents();
        
        // 连接结果列表的点击事件
        connect(table, &QTableWidget::itemClicked, [&](QTableWidgetItem *item) {
            int row = table->item(item->row(), 0)->text().toInt() - 1;
            dataTable->selectRow(row);
            dataTable->scrollToItem(dataTable->item(row, 0));
        }); });

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::updateDatabaseInfo()
{
    if (!db)
    {
        qDebug() << "数据库未连接，无法更新数据库信息";

        // 获取数据库信息面板
        QGroupBox *dbInfoGroup = nullptr;
        for (QObject *child : databaseTab->children())
        {
            if (child->inherits("QGroupBox"))
            {
                dbInfoGroup = qobject_cast<QGroupBox *>(child);
                break;
            }
        }

        // 隐藏数据库信息面板
        if (dbInfoGroup)
        {
            dbInfoGroup->setVisible(false);
        }

        return;
    }

    qDebug() << "开始更新数据库信息";

    // 获取数据库信息面板
    QGroupBox *dbInfoGroup = nullptr;
    for (QObject *child : databaseTab->children())
    {
        if (child->inherits("QGroupBox"))
        {
            dbInfoGroup = qobject_cast<QGroupBox *>(child);
            break;
        }
    }

    // 显示数据库信息面板
    if (dbInfoGroup)
    {
        dbInfoGroup->setVisible(true);
    }

    try
    {
        // 获取数据库信息
        QString dbName = QString::fromUtf8(db->fileName);
        QString dbVersion = QString::fromUtf8(reinterpret_cast<const char *>(db->dbVersion));
        QString dbOverview = QString::fromUtf8(reinterpret_cast<const char *>(db->dbOverview));
        QString totalPages = QString::number(db->totalPageNum);
        QString freePages = QString::number(db->firstFreePid);

        qDebug() << "数据库信息:";
        qDebug() << "  名称:" << dbName;
        qDebug() << "  版本:" << dbVersion;
        qDebug() << "  概述:" << dbOverview;
        qDebug() << "  总页数:" << totalPages;
        qDebug() << "  空闲页:" << freePages;

        // 更新标签文本
        QList<QLabel *> labels = databaseTab->findChildren<QLabel *>();
        for (QLabel *label : labels)
        {
            if (label->text().startsWith("数据库名称:"))
            {
                label->setText("数据库名称: " + dbName);
            }
            else if (label->text().startsWith("数据库版本:"))
            {
                label->setText("数据库版本: " + dbVersion);
            }
            else if (label->text().startsWith("数据库概述:"))
            {
                label->setText("数据库概述: " + dbOverview);
            }
            else if (label->text().startsWith("总页数:"))
            {
                label->setText("总页数: " + totalPages);
            }
            else if (label->text().startsWith("空闲页:"))
            {
                label->setText("空闲页: " + freePages);
            }
        }
    }
    catch (const std::exception &e)
    {
        qDebug() << "更新数据库信息时发生异常:" << e.what();
    }
    catch (...)
    {
        qDebug() << "更新数据库信息时发生未知异常";
    }
}

void MainWindow::onVacuumDatabase()
{
    // 检查是否连接了数据库
    if (!db)
    {
        QMessageBox::warning(this, "警告", "请先连接数据库");
        return;
    }

    // 断开现有连接
    QString currentDbPath;
    if (dbPathLabel)
    {
        QString labelText = dbPathLabel->text();
        if (labelText.startsWith("路径: "))
        {
            currentDbPath = labelText.mid(4); // 去掉前缀 "路径: "
        }
    }

    if (currentDbPath.isEmpty())
    {
        QMessageBox::warning(this, "警告", "无法获取当前数据库路径");
        return;
    }

    // 选择保存压缩后数据库的路径
    QString newDbPath = QFileDialog::getSaveFileName(this,
                                                     tr("保存压缩后的数据库"),
                                                     QFileInfo(currentDbPath).path(), // 使用原数据库所在的目录
                                                     tr("数据库文件 (*.db *.dat)"));

    if (newDbPath.isEmpty())
    {
        return; // 用户取消了操作
    }

    // 确保文件扩展名正确
    QFileInfo fileInfo(newDbPath);
    QString suffix = fileInfo.suffix().toLower();
    if (suffix != "db" && suffix != "dat")
    {
        newDbPath += ".db"; // 默认使用.db扩展名
    }

    // 需要先断开连接才能压缩数据库
    if (db)
    {
        onDisconnectDB();
    }

    // 执行压缩操作
    QByteArray oldPathBytes = currentDbPath.toLocal8Bit();
    QByteArray newPathBytes = newDbPath.toLocal8Bit();
    long contractSize = 0;

    // 显示进度对话框或状态栏提示
    statusBar->showMessage("正在压缩数据库，请稍候...");
    QApplication::processEvents(); // 确保UI更新

    int result = vacuum(oldPathBytes.data(), newPathBytes.data(), &contractSize);

    if (result == 0)
    {
        // 压缩成功
        QString successMsg = QString("数据库压缩成功，节省了 %1 字节的空间。").arg(contractSize);
        QMessageBox::information(this, "成功", successMsg);
        statusBar->showMessage(successMsg, 5000);

        // 询问是否打开压缩后的数据库
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  "打开新数据库",
                                                                  "是否打开压缩后的数据库？",
                                                                  QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            // 打开新的数据库
            db = new GNCDB();
            int rc;

            QByteArray dbPathBytes = newDbPath.toLocal8Bit();
            char *dbPathChar = dbPathBytes.data();

            if ((rc = GNCDB_open(&db, dbPathChar)) == 0)
            {
                statusBar->showMessage("数据库连接成功");
                if (tableNameLabel)
                {
                    tableNameLabel->setText("表: 未选择");
                }
                if (dbPathLabel)
                {
                    dbPathLabel->setText(QString("路径: %1").arg(newDbPath));
                }
                if (dbTitleLabel)
                {
                    dbTitleLabel->setText(QFileInfo(newDbPath).baseName());
                }

                // 清空并更新表列表
                tableTree->clear();
                updateTableList();
                updateDatabaseInfo(); // 更新数据库信息
            }
            else
            {
                QString errorMsg = QString("数据库连接失败，错误代码: %1").arg(rc);
                showError(errorMsg);
                if (db)
                {
                    delete db;
                    db = nullptr;
                }
            }
        }
    }
    else
    {
        // 压缩失败
        QString errorMsg = QString("数据库压缩失败，错误代码: %1").arg(result);
        showError(errorMsg);
        statusBar->showMessage(errorMsg, 5000);
    }
}

void MainWindow::refreshTable()
{
    if (!dataTable)
        return;

    // 执行刷新逻辑
    QString tableName = dataTable->property("currentTable").toString();
    if (!tableName.isEmpty())
    {
        showTableData(tableName);

        // 如果有数据，选中第一行
        if (dataTable->rowCount() > 0)
        {
            dataTable->selectRow(0);
        }
    }
}

// 保存窗口设置到配置文件
void MainWindow::saveWindowSettings()
{
    qDebug() << "保存窗口设置到" << settingsFilePath;
    
    QSettings settings(settingsFilePath, QSettings::IniFormat);
    
    // 保存窗口大小和位置
    settings.beginGroup("Window");
    settings.setValue("Size", size());
    settings.setValue("Position", pos());
    settings.setValue("Maximized", isMaximized());
    settings.endGroup();
    
    // 保存分割器位置（如果存在）
    if (mainSplitter)
    {
        settings.beginGroup("Splitter");
        settings.setValue("Sizes", QVariant::fromValue(mainSplitter->sizes()));
        
        // 保存树形图的可见状态
        QList<int> sizes = mainSplitter->sizes();
        settings.setValue("TreeVisible", sizes[0] > 0);
        
        // 如果树形图当前不可见，保存之前的大小
        if (sizes[0] == 0) {
            int savedSize = mainSplitter->property("savedTreeSize").toInt();
            if (savedSize > 0) {
                settings.setValue("SavedTreeSize", savedSize);
            }
        }
        
        settings.endGroup();
    }
    
    // 保存标签页的可见状态
    if (rightTabWidget)
    {
        settings.beginGroup("Tabs");
        for (int i = 0; i < rightTabWidget->count(); ++i)
        {
            settings.setValue(QString("Tab%1Visible").arg(i), rightTabWidget->isTabVisible(i));
        }
        settings.endGroup();
    }
    
    settings.sync();
    qDebug() << "窗口设置已保存";
}

// 从配置文件加载窗口设置
void MainWindow::loadWindowSettings()
{
    qDebug() << "从" << settingsFilePath << "加载窗口设置";
    
    QSettings settings(settingsFilePath, QSettings::IniFormat);
    
    // 恢复窗口大小和位置
    settings.beginGroup("Window");
    QSize savedSize = settings.value("Size", QSize(1000, 700)).toSize();
    QPoint savedPos = settings.value("Position", QPoint(100, 100)).toPoint();
    bool wasMaximized = settings.value("Maximized", false).toBool();
    settings.endGroup();
    
    if (wasMaximized)
    {
        showMaximized();
    }
    else
    {
        resize(savedSize);
        move(savedPos);
    }
    
    // 分割器位置和树形图可见性将在initUI中恢复，因为此时mainSplitter还未创建
    settings.beginGroup("Splitter");
    bool treeVisible = settings.value("TreeVisible", true).toBool();
    int savedTreeSize = settings.value("SavedTreeSize", 300).toInt();
    settings.endGroup();
    
    // 保存这些值为属性，以便在initUI中使用
    setProperty("settingsTreeVisible", treeVisible);
    setProperty("settingsSavedTreeSize", savedTreeSize);
}

// 创建自定义标签页关闭按钮
QWidget* MainWindow::createTabCloseButton()
{
    // 创建按钮
    QToolButton* closeButton = new QToolButton();
    closeButton->setFixedSize(16, 16);
    
    // 创建黑色X图标
    QPixmap normalPixmap(16, 16);
    normalPixmap.fill(Qt::transparent);
    QPainter normalPainter(&normalPixmap);
    normalPainter.setRenderHint(QPainter::Antialiasing);
    normalPainter.setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    normalPainter.drawLine(4, 4, 12, 12);
    normalPainter.drawLine(12, 4, 4, 12);
    
    // 设置图标
    closeButton->setIcon(QIcon(normalPixmap));
    closeButton->setIconSize(QSize(16, 16));
    
    // 设置样式，使用伪状态(pseudo-state)实现悬停效果
    closeButton->setStyleSheet(R"(
        QToolButton {
            background: transparent;
            border: none;
        }
        QToolButton:hover {
            background: #D41123;
            border-radius: 6px;
        }
        QToolButton:pressed {
            background: #CC1123;
        }
    )");
    
    // 当按钮被悬停时，修改图标颜色
    class EventFilter : public QObject {
    public:
        EventFilter(QToolButton* button) : QObject(button), btn(button) {
            // 创建普通图标
            QPixmap normal(16, 16);
            normal.fill(Qt::transparent);
            QPainter np(&normal);
            np.setRenderHint(QPainter::Antialiasing);
            np.setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            np.drawLine(5, 5, 11, 11);
            np.drawLine(11, 5, 5, 11);
            normalIcon = QIcon(normal);
            
            // 创建悬停图标（白色X）
            QPixmap hover(16, 16);
            hover.fill(Qt::transparent);
            QPainter hp(&hover);
            hp.setRenderHint(QPainter::Antialiasing);
            hp.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            hp.drawLine(5, 5, 11, 11);
            hp.drawLine(11, 5, 5, 11);
            hoverIcon = QIcon(hover);
            
            btn->setIcon(normalIcon);
        }
        
        bool eventFilter(QObject* watched, QEvent* event) override {
            if (watched == btn) {
                if (event->type() == QEvent::Enter) {
                    btn->setIcon(hoverIcon);
                } else if (event->type() == QEvent::Leave) {
                    btn->setIcon(normalIcon);
                }
            }
            return QObject::eventFilter(watched, event);
        }
        
    private:
        QToolButton* btn;
        QIcon normalIcon;
        QIcon hoverIcon;
    };
    
    // 安装事件过滤器
    EventFilter* filter = new EventFilter(closeButton);
    closeButton->installEventFilter(filter);
    
    // 设置光标样式
    closeButton->setCursor(Qt::PointingHandCursor);
    
    // 连接点击信号
    connect(closeButton, &QToolButton::clicked, [this, closeButton]() {
        for (int i = 0; i < rightTabWidget->count(); i++) {
            if (rightTabWidget->tabBar()->tabButton(i, QTabBar::RightSide) == closeButton) {
                emit rightTabWidget->tabCloseRequested(i);
                break;
            }
        }
    });
    
    return closeButton;
}


