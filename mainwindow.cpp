#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./database/dbmanager.h"
#include "./ui/tabledialog.h"
#include "gncdb.h"
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

// 定义一个结构体来存储表名
struct TableInfo {
    QString tableName;
};

// 定义一个全局变量来存储表名列表
QVector<TableInfo> tableList;

// C++版本的callback函数
int cppCallback(void *data, int argc, char **/*azColName*/, char **argv) {
    QVector<TableInfo> *tables = static_cast<QVector<TableInfo>*>(data);
    
    // 添加调试信息
    qDebug() << "Callback被调用，列数:" << argc;
    
    // 假设第一列是表名
    if (argc > 0 && argv[0]) {
        TableInfo table;
        table.tableName = QString::fromUtf8(argv[0]);
        tables->append(table);
        qDebug() << "添加表名:" << table.tableName;
    }
    
    return 0;
}

// 修改callback函数以适应GNCDB_select
int selectCallback(void *data, int argc, char **/*azColName*/, char **argv) {
    QVector<TableInfo> *tables = static_cast<QVector<TableInfo>*>(data);
    
    // 添加调试信息
    qDebug() << "SelectCallback被调用，列数:" << argc;
    
    // 假设第一列是表名
    if (argc > 0 && argv[0]) {
        TableInfo table;
        table.tableName = QString::fromUtf8(argv[0]);
        tables->append(table);
        qDebug() << "添加表名:" << table.tableName;
    }
    
    return 0;
}

// 用于SQL查询结果的callback函数
int sqlResultCallback(void *data, int argc, char **azColName, char **argv) {
    SQLResult *result = static_cast<SQLResult*>(data);
    
    qDebug() << "sqlResultCallback被调用，列数:" << argc;
    
    // 如果是第一次调用，保存列名
    if (result->columnNames.isEmpty()) {
        qDebug() << "保存列名:";
        for (int i = 0; i < argc; i++) {
            QString colName = QString::fromUtf8(azColName[i]);
            result->columnNames.append(colName);
            qDebug() << "  列" << i << ":" << colName;
        }
    }
    
    // 保存行数据
    QStringList row;
    qDebug() << "保存行数据:";
    for (int i = 0; i < argc; i++) {
        QString value = argv[i] ? QString::fromUtf8(argv[i]) : "NULL";
        row.append(value);
        qDebug() << "  列" << i << ":" << value;
    }
    result->rows.append(row);
    
    return 0;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , db(nullptr)  // 确保初始化为 nullptr
{
    dbManager = new DBManager();
    ui->setupUi(this);
    initUI();
    initConnections();
}

MainWindow::~MainWindow()
{
    if (db) {
        GNCDB_close(&db);  // 确保在析构时正确关闭数据库
        db = nullptr;
    }
    delete ui;
    delete dbManager;
}

void MainWindow::initUI()
{
    setWindowIcon(QIcon(":/app_icon.png"));  
    // 创建菜单栏
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // 文件菜单
    fileMenu = menuBar->addMenu(tr("文件(&F)"));
    
    // 编辑菜单
    editMenu = menuBar->addMenu(tr("编辑(&E)"));
    
    // 数据库菜单
    databaseMenu = menuBar->addMenu(tr("数据库(&D)"));
    QAction *openDbAction = databaseMenu->addAction(tr("打开数据库"), this, &MainWindow::onConnectDB);
    openDbAction->setShortcut(QKeySequence("Ctrl+O"));
    QAction *disconnectDbAction = databaseMenu->addAction(tr("断开连接"), this, &MainWindow::onDisconnectDB);
    disconnectDbAction->setShortcut(QKeySequence("Ctrl+D"));
    
    // 表管理菜单
    tableMenu = menuBar->addMenu(tr("表管理(&T)"));
    tableMenu->addAction(tr("新建表"), this, &MainWindow::onCreateTable);
    tableMenu->addAction(tr("删除表"), this, &MainWindow::onDropTable);
    tableMenu->addAction(tr("插入行"), this, &MainWindow::onAddRow);
    
    // 视图菜单
    viewMenu = menuBar->addMenu(tr("视图(&V)"));
    
    // SQL菜单
    sqlMenu = menuBar->addMenu(tr("SQL(&S)"));
    
    // 事务菜单
    transMenu = menuBar->addMenu(tr("事务(&X)"));
    
    // 工具菜单
    toolsMenu = menuBar->addMenu(tr("工具(&O)"));
    
    // 帮助菜单
    helpMenu = menuBar->addMenu(tr("帮助(&H)"));

    // 创建可停靠工具栏
    mainToolBar = addToolBar(tr("主工具栏"));
    mainToolBar->setMovable(true);
    mainToolBar->setFloatable(true);
    
    // 创建图表空间按钮
    QToolButton *chartButton = new QToolButton(this);
    chartButton->setText(tr("图表空间"));
    chartButton->setIcon(QIcon(":/icons/chart.png")); // 需要添加相应的图标资源
    chartButton->setPopupMode(QToolButton::MenuButtonPopup);
    
    // 创建图表空间菜单
    QMenu *chartMenu = new QMenu(this);
    chartButton->setMenu(chartMenu);
    
    // 将图表按钮添加到工具栏
    mainToolBar->addWidget(chartButton);
    
    // 添加分隔符
    mainToolBar->addSeparator();
    
    // 预留动态工具区域（后续可以根据需要动态添加）
    
    // 创建主分割器
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // 左侧树形结构（30%宽度）
    tableTree = new QTreeWidget(mainSplitter);
    tableTree->setHeaderLabel("数据库表");
    tableTree->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 右侧标签页（70%宽度）
    rightTabWidget = new QTabWidget(mainSplitter);
    
    // 数据标签
    dataTable = new QTableWidget(rightTabWidget);
    rightTabWidget->addTab(dataTable, "数据");
    
    // SQL标签
    QWidget *sqlWidget = new QWidget(rightTabWidget);
    QHBoxLayout *sqlMainLayout = new QHBoxLayout(sqlWidget);
    
    // 左侧SQL输入区域
    QVBoxLayout *sqlLeftLayout = new QVBoxLayout();
    sqlEditor = new QTextEdit(sqlWidget);
    sqlEditor->setPlaceholderText(tr("在此输入SQL语句..."));
    QPushButton *executeButton = new QPushButton(tr("执行SQL"), sqlWidget);
    sqlLeftLayout->addWidget(sqlEditor);
    sqlLeftLayout->addWidget(executeButton);
    
    // 右侧结果显示区域
    QVBoxLayout *sqlRightLayout = new QVBoxLayout();
    sqlResultDisplay = new QTableWidget(sqlWidget);
    sqlResultDisplay->setEditTriggers(QTableWidget::NoEditTriggers); // 设置为只读
    sqlRightLayout->addWidget(sqlResultDisplay);
    
    // 添加分割器
    QSplitter *sqlSplitter = new QSplitter(Qt::Horizontal, sqlWidget);
    QWidget *leftWidget = new QWidget(sqlSplitter);
    QWidget *rightWidget = new QWidget(sqlSplitter);
    leftWidget->setLayout(sqlLeftLayout);
    rightWidget->setLayout(sqlRightLayout);
    sqlSplitter->addWidget(leftWidget);
    sqlSplitter->addWidget(rightWidget);
    
    // 设置分割器的初始比例（4:6）
    sqlSplitter->setStretchFactor(0, 4);
    sqlSplitter->setStretchFactor(1, 6);
    
    sqlMainLayout->addWidget(sqlSplitter);
    rightTabWidget->addTab(sqlWidget, tr("SQL"));
    
    // 数据库标签（暂时留空）
    databaseTab = new QWidget(rightTabWidget);
    rightTabWidget->addTab(databaseTab, "数据库");

    // 设置分割器初始比例（30:70）
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 7);

    // 状态栏
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("未连接数据库");

    // 连接信号
    connect(executeButton, &QPushButton::clicked, this, &MainWindow::onExecuteSQL);
}

void MainWindow::initConnections()
{
    // 修改表选择事件，只在点击表项时触发
    connect(tableTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item) {
        // 如果点击的是列信息项，则不执行任何操作
        if (item->parent() != nullptr) {
            return;
        }
        onTableSelected(item, 0);
    });

    // 添加展开事件处理
    connect(tableTree, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item) {
        // 无论是否有临时子节点，都尝试加载列信息
        if (item->parent() == nullptr) {
            loadTableColumns(item);
        }
    });
    
    connect(dataTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::showTableContextMenu);
    
    // 连接SQL执行按钮的点击事件
    QPushButton *executeButton = ui->executeButton;
    if (executeButton) {
        qDebug() << "找到执行按钮，正在连接信号...";
        connect(executeButton, &QPushButton::clicked, this, &MainWindow::onExecuteSQL);
    } else {
        qDebug() << "警告：未找到执行按钮！";
    }
}

void MainWindow::onConnectDB()
{
    if (db) {
        QMessageBox::warning(this, "警告", "已经连接到数据库");
        return;
    }

    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("打开数据库文件"),
        QDir::currentPath(),
        tr("数据库文件 (*.db *.dat)"));

    if (fileName.isEmpty()) {
        return;
    }

    // 获取数据库文件所在目录
    QFileInfo fileInfo(fileName);
    QString dbDir = fileInfo.absolutePath();
    
    // 检查log目录是否存在
    QDir dir(dbDir);
    if (!dir.exists("log")) {
        // 创建log目录
        if (!dir.mkdir("log")) {
            QMessageBox::warning(this, "警告", "无法创建log目录");
            return;
        }
        qDebug() << "已创建log目录:" << dbDir + "/log";
    }

    db = new GNCDB();
    int rc;
    
    qDebug() << "尝试打开数据库文件：" << fileName;
    
    // 将 QString 转换为 char* 类型
    QByteArray dbPathBytes = fileName.toLocal8Bit();
    char* dbPathChar = dbPathBytes.data();
    
    if ((rc = GNCDB_open(&db, dbPathChar)) == 0) {
        statusBar->showMessage("数据库连接成功");
        updateTableList();
    } else {
        QString errorMsg = QString("数据库连接失败，错误代码: %1，文件路径: %2").arg(rc).arg(fileName);
        showError(errorMsg);
        delete db;
        db = nullptr;
    }
}

void MainWindow::onDisconnectDB()
{
    if (!db) {
        QMessageBox::warning(this, "警告", "未连接到数据库");
        return;
    }

    GNCDB_close(&db);
    db = nullptr;
    
    // 清空左侧表树
    tableTree->clear();
    
    // 清空右侧数据表格
    dataTable->clear();
    dataTable->setRowCount(0);
    dataTable->setColumnCount(0);
    
    // 清空SQL编辑器
    sqlEditor->clear();
    
    statusBar->showMessage("未连接数据库");
}

void MainWindow::onExecuteSQL()
{
    if (!db) {
        showError("请先连接数据库");
        return;
    }

    QString sql = sqlEditor->toPlainText().trimmed();
    if (sql.isEmpty()) {
        showError("请输入SQL语句");
        return;
    }

    qDebug() << "准备执行SQL语句:" << sql;
    
    // 创建SQL结果对象
    SQLResult result;
    
    // 执行SQL语句
    char *errmsg = nullptr;
    qDebug() << "调用GNCDB_exec...";
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    qDebug() << "GNCDB_exec返回码:" << rc;
    
    if (rc != 0) {
        QString errorMsg = QString("SQL执行失败: %1").arg(errmsg ? errmsg : "未知错误");
        qDebug() << "错误代码:" << rc;
        qDebug() << "错误信息:" << errorMsg;
        showError(errorMsg);
        if (errmsg) {
            free(errmsg);
        }
        return;
    }

    // 清空并设置结果表格
    sqlResultDisplay->clear();
    sqlResultDisplay->setRowCount(0);
    sqlResultDisplay->setColumnCount(result.columnNames.size());
    sqlResultDisplay->setHorizontalHeaderLabels(result.columnNames);

    // 设置行数据
    sqlResultDisplay->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); row++) {
        const QStringList &rowData = result.rows[row];
        for (int col = 0; col < rowData.size(); col++) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[col]);
            sqlResultDisplay->setItem(row, col, item);
        }
    }

    // 自动调整列宽
    sqlResultDisplay->resizeColumnsToContents();

    // 更新状态栏
    QString statusMsg = QString("查询完成，返回 %1 行数据").arg(result.rows.size());
    statusBar->showMessage(statusMsg);
}

void MainWindow::onTableSelected(QTreeWidgetItem *item, int /*column*/) {
    if (!db || !item) return;
    
    currentTable = item->text(0);
    qDebug() << "选中表:" << currentTable;
    
    // 断开之前的信号连接
    disconnect(dataTable, &QTableWidget::cellChanged, this, nullptr);
    
    // 首先获取表结构信息
    SQLResult schemaResult;
    QString schemaSql = QString("SELECT * FROM schema WHERE tableName = '%1' ORDER BY columnIndex").arg(currentTable);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, schemaSql.toUtf8().constData(), sqlResultCallback, &schemaResult, &errmsg);
    
    if (rc != 0) {
        QString errorMsg = QString("获取表结构失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    // 提取列名并过滤系统列
    QStringList columnNames;
    for (const QStringList &row : schemaResult.rows) {
        QString colName = row[1];  // columnName
        if (colName != "rowId" && colName != "createTime" && colName != "updateTime") {
            columnNames.append(colName);
        }
    }
    
    // 设置列名
    currentColumnNames = columnNames;
    
    // 设置表格的列
    dataTable->clear();
    dataTable->setColumnCount(columnNames.size());
    dataTable->setHorizontalHeaderLabels(columnNames);
    
    // 查询数据
    SQLResult dataResult;
    QString dataSql = QString("SELECT * FROM %1").arg(currentTable);
    rc = GNCDB_exec(db, dataSql.toUtf8().constData(), sqlResultCallback, &dataResult, &errmsg);
    
    if (rc != 0) {
        QString errorMsg = QString("查询表 %1 失败: %2").arg(currentTable).arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }
    
    // 显示数据（如果有）
    dataTable->setRowCount(dataResult.rows.size());
    for (int row = 0; row < dataResult.rows.size(); row++) {
        const QStringList &rowData = dataResult.rows[row];
        int displayCol = 0;
        for (int col = 0; col < rowData.size(); col++) {
            if (dataResult.columnNames[col] != "rowId" && 
                dataResult.columnNames[col] != "createTime" && 
                dataResult.columnNames[col] != "updateTime") {
                QTableWidgetItem *item = new QTableWidgetItem(rowData[col]);
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                item->setData(Qt::UserRole, rowData[col]);
                dataTable->setItem(row, displayCol++, item);
            }
        }
    }
    
    // 调整列宽
    dataTable->resizeColumnsToContents();
    
    // 显示状态信息
    QString statusMsg = QString("查询完成，返回 %1 行数据").arg(dataResult.rows.size());
    statusBar->showMessage(statusMsg);
    
    // 连接单元格编辑完成的信号
    connect(dataTable, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (!db || currentTable.isEmpty()) {
            qDebug() << "数据库未连接或表名为空";
            return;
        }
        
        QTableWidgetItem *item = dataTable->item(row, column);
        if (!item) {
            qDebug() << "表格项为空";
            return;
        }
        
        QString newValue = item->text();
        QString oldValue = item->data(Qt::UserRole).toString();
        
        qDebug() << "编辑单元格 - 行:" << row << "列:" << column;
        qDebug() << "列名:" << currentColumnNames[column];
        qDebug() << "原值:" << oldValue << "新值:" << newValue;
        
        // 如果值没有改变，直接返回
        if (newValue == oldValue) {
            qDebug() << "值未改变，忽略更新";
            return;
        }
        
        // 获取主键值
        QTableWidgetItem *primaryKeyItem = dataTable->item(row, 0);
        if (!primaryKeyItem) {
            qDebug() << "主键项为空";
            return;
        }
        QString primaryKeyValue = primaryKeyItem->text();
        qDebug() << "主键值:" << primaryKeyValue;
        
        // 判断数据类型
        bool isInteger = true;
        for (const QChar &c : newValue) {
            if (!c.isDigit() && c != '-') {
                isInteger = false;
                break;
            }
        }
        
        qDebug() << "数据类型判断 - 是否为整数:" << isInteger;
        
        // 根据类型决定是否添加引号
        QString formattedValue = newValue;
        if (!isInteger) {
            formattedValue = "'" + newValue + "'";
        }
        
        // 构建UPDATE语句
        QString sql = QString("UPDATE %1 SET %2 = %3 WHERE %4 = %5")
            .arg(currentTable)
            .arg(currentColumnNames[column])
            .arg(formattedValue)
            .arg(currentColumnNames[0])  // 使用第一列作为主键
            .arg(primaryKeyValue);
            
        qDebug() << "执行UPDATE语句:" << sql;
            
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
        
        if (rc != 0) {
            QString errorMsg = QString("更新数据失败: %1").arg(errmsg ? errmsg : "未知错误");
            qDebug() << "错误代码:" << rc;
            qDebug() << "错误信息:" << errorMsg;
            showError(errorMsg);
            if (errmsg) free(errmsg);
            // 恢复原值
            item->setText(oldValue);
            return;
        }
        
        qDebug() << "更新成功";
        // 保存新值
        item->setData(Qt::UserRole, newValue);
    });
}

void MainWindow::onShowTables()
{
    if (!db) {
        showError("请先连接数据库");
        return;
    }
    updateTableList();
}

void MainWindow::onRefreshTables()
{
    if (!db) {
        showError("请先连接数据库");
        return;
    }
    updateTableList();
}

void MainWindow::updateTableList()
{
    if (!db) {
        qDebug() << "数据库未连接，无法更新表列表";
        return;
    }

    // 清空表树
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
    if (rc2 != 0) {
        QString errorMsg = QString("获取表列表失败，错误代码: %1").arg(rc2);
        qDebug() << errorMsg;
        showError(errorMsg);
        return;
    }

    qDebug() << "SQL查询执行成功，找到" << tableList.size() << "个表";

    // 将表名添加到树形控件中
    for (const TableInfo &table : tableList) {
        QTreeWidgetItem *tableItem = new QTreeWidgetItem(tableTree);
        tableItem->setText(0, table.tableName);
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
    
    // 查询表结构 - 修改查询语句以获取完整的列信息
    SQLResult result;
    QString sql = QString("SELECT * FROM schema WHERE tableName = '%1' ORDER BY columnIndex").arg(tableName);
    qDebug() << "[DEBUG] 执行查询表结构的SQL语句:" << sql;
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    
    if (rc == 0) {
        qDebug() << "[DEBUG] 查询表结构成功，返回行数:" << result.rows.size();
        // 为每一列创建子节点
        for (const QStringList &row : result.rows) {
            qDebug() << "[DEBUG] 查询结果行:" << row;
            if (row.size() < 9) { // 确保有足够的列
                qDebug() << "[WARNING] 查询结果行数据不足，跳过：" << row;
                continue;
            }
            
            QString colName = row[1];      // columnName
            QString colTypeStr = getFieldTypeName(row[4].toInt()); // 转换类型ID为类型名
            int colLength = row[5].toInt(); // columnLength
            QString isPrimary = row[8];     // isPrimaryKey
            
            // 跳过系统信息列
            if (colName == "rowId" || colName == "createTime" || colName == "updateTime") {
                qDebug() << "[DEBUG] 跳过系统列:" << colName;
                continue;
            }
            
            // 构建列信息显示文本
            QString columnInfo = QString("%1 (%2)").arg(colName).arg(colTypeStr);
            if (colTypeStr == "VARCHAR") {
                columnInfo += QString("(%1)").arg(colLength);
            }
            if (isPrimary == "1") {
                columnInfo += " [PK]";
            }
            
            QTreeWidgetItem *columnItem = new QTreeWidgetItem(tableItem);
            columnItem->setText(0, columnInfo);
            columnItem->setFlags(columnItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
            qDebug() << "[DEBUG] 添加列项:" << columnInfo;
        }
        
        qDebug() << "[DEBUG] 列信息加载完成";
    } else {
        qDebug() << "[ERROR] 加载表列信息失败，错误码:" << rc << "，错误信息:" << (errmsg ? errmsg : "未知错误");
        if (errmsg) {
            free(errmsg);
        }
    }
}

void MainWindow::showTableData(const QString &/*tableName*/)
{
    if (!db) return;

    dataTable->clear();
    // TODO: 使用gncdblib的API获取表数据
    // 这里需要根据gncdblib的API来实现
}

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, "错误", message);
    statusBar->showMessage(message);
}

void MainWindow::showCreateTableDialog()
{
    if (!db) {
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
    QList<QHBoxLayout*> columnLayouts;
    QList<QLineEdit*> columnNames;
    QList<QComboBox*> columnTypes;
    QList<QSpinBox*> columnLengths;

    // 添加列按钮
    QPushButton *addColumnBtn = new QPushButton("添加列", &dialog);
    layout->addWidget(addColumnBtn);

    // 确定和取消按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    layout->addWidget(buttonBox);

    // 添加第一列的输入控件
    auto addColumnControls = [&dialog, &columnNames, &columnTypes, &columnLengths, &columnLayouts, columnsLayout]() {
        QHBoxLayout *columnLayout = new QHBoxLayout();
        
        QLineEdit *colName = new QLineEdit(&dialog);
        colName->setPlaceholderText("列名");
        
        QComboBox *colType = new QComboBox(&dialog);
        colType->addItems({"INT", "FLOAT", "VARCHAR", "TEXT", "DATE", "DATETIME"}); // 只保留支持的类型
        
        QSpinBox *colLength = new QSpinBox(&dialog);
        colLength->setRange(1, 255);
        colLength->setValue(32);
        colLength->setEnabled(false);

        // 当类型改变时更新长度输入框的状态
        QObject::connect(colType, &QComboBox::currentTextChanged, [colLength](const QString &text) {
            colLength->setEnabled(text == "VARCHAR"); // 只有 VARCHAR 需要长度参数
        });

        columnLayout->addWidget(colName);
        columnLayout->addWidget(colType);
        columnLayout->addWidget(colLength);

        columnNames.append(colName);
        columnTypes.append(colType);
        columnLengths.append(colLength);
        columnLayouts.append(columnLayout);
        columnsLayout->addLayout(columnLayout);
    };

    // 添加第一列
    addColumnControls();
    layout->addWidget(columnsWidget);

    // 连接添加列按钮
    QObject::connect(addColumnBtn, &QPushButton::clicked, [addColumnControls]() {
        addColumnControls();
    });

    // 连接确定按钮
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
        QString tableName = nameEdit->text().trimmed();
        if (tableName.isEmpty()) {
            showError("请输入表名");
            return;
        }

        // 构建CREATE TABLE语句
        QString sql = QString("CREATE TABLE %1 (").arg(tableName);
        QStringList columns;
        
        for (int i = 0; i < columnNames.size(); i++) {
            QString colName = columnNames[i]->text().trimmed();
            QString colType = columnTypes[i]->currentText();
            
            if (colType == "VARCHAR") {
                int length = columnLengths[i]->value();
                colType = QString("CHAR(%1)").arg(length); // 使用 CHAR(length) 格式
            }

            if (colName.isEmpty()) continue;

            QString colDef = colName + " " + colType;
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
            QString errorMsg = QString("创建表失败: %1").arg(errmsg ? errmsg : "未知错误");
            showError(errorMsg);
            if (errmsg) free(errmsg);
            return;
        }

        // 刷新表列表
        updateTableList();
        dialog.accept();
    });

    // 连接取消按钮
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 显示对话框
    dialog.exec();
}

void MainWindow::onDropTable()
{
    if (!db) {
        showError("请先连接数据库");
        return;
    }

    // 获取当前选中的表
    QTreeWidgetItem *currentItem = tableTree->currentItem();
    if (!currentItem) {
        showError("请先选择要删除的表");
        return;
    }

    QString tableName = currentItem->text(0);
    
    // 确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "确认删除", 
        QString("确定要删除表 %1 吗？此操作不可恢复！").arg(tableName),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // 构建DROP TABLE语句
        QString sql = QString("DROP TABLE %1").arg(tableName);
        qDebug() << "执行删表SQL:" << sql;

        // 执行删表语句
        char *errmsg = nullptr;
        int rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
        
        if (rc != 0) {
            QString errorMsg = QString("删除表失败: %1").arg(errmsg ? errmsg : "未知错误");
            showError(errorMsg);
            if (errmsg) free(errmsg);
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
    if (!db || currentTable.isEmpty()) return;

    QMenu contextMenu(this);
    QAction *addAction = contextMenu.addAction("添加行");
    QAction *editAction = contextMenu.addAction("编辑行");
    QAction *deleteAction = contextMenu.addAction("删除行");

    QAction *selectedAction = contextMenu.exec(dataTable->viewport()->mapToGlobal(pos));
    if (!selectedAction) return;

    if (selectedAction == addAction) {
        onAddRow();
    } else if (selectedAction == editAction) {
        onEditRow();
    } else if (selectedAction == deleteAction) {
        onDeleteRow();
    }
}

void MainWindow::onAddRow()
{
    if (!db || currentTable.isEmpty()) return;
    showEditRowDialog();
}

void MainWindow::onEditRow()
{
    if (!db || currentTable.isEmpty()) return;
    
    QList<QTableWidgetItem*> selectedItems = dataTable->selectedItems();
    if (selectedItems.isEmpty()) {
        showError("请先选择要编辑的行");
        return;
    }

    int row = selectedItems.first()->row();
    QStringList rowData;
    for (int i = 0; i < dataTable->columnCount(); ++i) {
        QTableWidgetItem *item = dataTable->item(row, i);
        rowData.append(item ? item->text() : "");
    }
    
    showEditRowDialog(rowData);
}

void MainWindow::onDeleteRow()
{
    if (!db || currentTable.isEmpty()) return;
    
    QList<QTableWidgetItem*> selectedItems = dataTable->selectedItems();
    if (selectedItems.isEmpty()) {
        showError("请先选择要删除的行");
        return;
    }

    int row = selectedItems.first()->row();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "确认删除", 
        "确定要删除选中的行吗？此操作不可恢复！",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        executeDeleteSQL(currentTable, row);
    }
}

QString MainWindow::getFieldTypeName(int typeId) {
    switch (typeId) {
        case FIELDTYPE_INTEGER: return "INTEGER";
        case FIELDTYPE_REAL: return "REAL";
        case FIELDTYPE_VARCHAR: return "VARCHAR";
        case FIELDTYPE_BLOB: return "BLOB";
        case FIELDTYPE_DATE: return "DATE";
        case FIELDTYPE_DATETIME: return "DATETIME";
        case FIELDTYPE_TEXT: return "TEXT";
        default: return "UNKNOWN";
    }
}

void MainWindow::showEditRowDialog(const QStringList &rowData)
{
    QDialog dialog(this);
    dialog.setWindowTitle(rowData.isEmpty() ? "添加行" : "编辑行");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QList<QLineEdit*> valueEdits;
    QMap<int, FieldType> columnFieldTypes; // 用于存储每列的数据类型
    
    // 获取表的列信息
    SQLResult result;
    QString sql = QString("SELECT * FROM schema WHERE tableName = '%1'").arg(currentTable);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    
    // 添加调试信息
    qDebug() << "执行查询schema语句:" << sql;
    qDebug() << "返回码:" << rc;
    if (errmsg) {
        qDebug() << "错误信息:" << errmsg;
    }
    qDebug() << "获取到的表结构:";
    for (const QStringList &row : result.rows) {
        qDebug() << row;
    }

    if (rc != 0) {
        QString errorMsg = QString("获取表结构失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    // 为每一列创建输入框
    for (const QStringList &row : result.rows) {
        QString colName = row[1]; // columnName
        int colTypeId = row[4].toInt(); // columnType (编号)
        QString colTypeStr = getFieldTypeName(colTypeId); // 映射为类型名称
        FieldType colType = getFieldTypeFromString(colTypeStr); // 获取列的数据类型

        // 跳过系统信息列
        if (colName == "rowId" || colName == "createTime" || colName == "updateTime") {
            continue;
        }
        
        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *label = new QLabel(QString("%1 (%2):").arg(colName).arg(colTypeStr), &dialog); // 显示列名和属性类型
        QLineEdit *edit = new QLineEdit(&dialog);
        
        if (!rowData.isEmpty() && row[3].toInt() < rowData.size()) { // columnIndex
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

    connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
        QStringList values;
        for (QLineEdit *edit : valueEdits) {
            values.append(edit->text());
        }

        if (rowData.isEmpty()) {
            executeInsertSQL(currentTable, values);
        } else {
            executeUpdateSQL(currentTable, values, dataTable->currentRow(), columnFieldTypes);
        }
        dialog.accept();
    });

    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

FieldType MainWindow::getFieldTypeFromString(const QString &typeStr) {
    if (typeStr == "INT" || typeStr == "INTEGER" || typeStr == "SMALLINT" || typeStr == "BIGINT") {
        return FIELDTYPE_INTEGER;
    } else if (typeStr == "REAL" || typeStr == "DOUBLE") {
        return FIELDTYPE_REAL;
    } else if (typeStr == "VARCHAR" || typeStr == "TEXT") {
        return FIELDTYPE_VARCHAR;//两种类型都统一为VARCHAR
    } else if (typeStr == "BLOB") {
        return FIELDTYPE_BLOB;
    } else if (typeStr == "DATE") {
        return FIELDTYPE_DATE;
    } else if (typeStr == "DATETIME" || typeStr == "TIMESTAMP") {
        return FIELDTYPE_DATETIME;
    } else {
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

    if (rc != 0) {
        QString errorMsg = QString("获取表结构失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    // 构建INSERT语句
    QString sql = QString("INSERT INTO %1 (").arg(tableName);
    QStringList columnNames;
    QStringList columnValues;

    int valueIndex = 0; // 用于匹配 values 的索引
    for (const QStringList &row : result.rows) {
        QString columnName = row[1]; // columnName
        QString columnType = row[4]; // columnType

        // 跳过系统信息列
        if (columnName == "rowId" || columnName == "createTime" || columnName == "updateTime") {
            continue;
        }

        columnNames.append(columnName);

        // 根据数据类型决定是否添加单引号
        int colTypeId = row[4].toInt(); // columnType (编号)
        QString colTypeStr = getFieldTypeName(colTypeId); // 映射为类型名称
        FieldType fieldType = getFieldTypeFromString(colTypeStr); // 确保 columnType 转换正确

        if (fieldType == FIELDTYPE_VARCHAR) {
            int colLength = row[5].toInt(); // 获取列长度
            QString escapedValue = values[valueIndex].trimmed(); // 确保值被修剪
            if (escapedValue.isEmpty()) {
                columnValues.append("NULL"); // 空字符串处理为 NULL
            } else {
                columnValues.append(QString("'%1'").arg(escapedValue.replace("'", "''"))); // 转义单引号并添加引号
            }
            // 添加长度信息
            columnValues.last() = QString("CHAR(%1)").arg(colLength) + columnValues.last();
        } else {
            columnValues.append(values[valueIndex]);
        }
        valueIndex++;
    }

    sql += columnNames.join(", ") + ") VALUES (" + columnValues.join(", ") + ")";
    sql += ";";

    // 添加调试信息
    qDebug() << "执行插入语句:" << sql;

    rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);

    if (rc != 0) {
        QString errorMsg = QString("插入数据失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    // 刷新表格数据
    onTableSelected(tableTree->currentItem(), 0);
}

void MainWindow::executeUpdateSQL(const QString &tableName, const QStringList &values, int rowIndex, const QMap<int, FieldType> &columnFieldTypes)
{
    // 获取列名
    SQLResult result;
    QString sql = QString("SELECT * FROM schema WHERE tableName = '%1'").arg(tableName);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    
    if (rc != 0) {
        QString errorMsg = QString("获取表结构失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    // 构建UPDATE语句
    QStringList setClauses;
    for (int i = 0; i < values.size(); ++i) {
        QString colName = result.rows[i][1];
        QString value = values[i];
        FieldType fieldType = columnFieldTypes[i]; // 获取列的数据类型
        
        // 根据数据类型决定是否添加引号
        if (fieldType == FIELDTYPE_INTEGER || fieldType == FIELDTYPE_REAL) {
            setClauses.append(QString("%1 = %2").arg(colName).arg(value));
        } else {
            setClauses.append(QString("%1 = '%2'").arg(colName).arg(value));
        }
    }

    // 获取主键值
    QString primaryKeyValue = dataTable->item(rowIndex, 0)->text();
    sql = QString("UPDATE %1 SET %2 WHERE %3 = '%4'")
        .arg(tableName)
        .arg(setClauses.join(", "))
        .arg(result.rows[0][1])
        .arg(primaryKeyValue);

    rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
    
    if (rc != 0) {
        QString errorMsg = QString("更新数据失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    // 刷新表格数据
    onTableSelected(tableTree->currentItem(), 0);
}

void MainWindow::executeDeleteSQL(const QString &tableName, int rowIndex)
{
    // 获取主键值
    QString primaryKeyValue = dataTable->item(rowIndex, 0)->text();
    
    // 获取主键列名
    SQLResult result;
    QString sql = QString("SELECT * FROM schema WHERE tableName = '%1'").arg(tableName);
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    
    // 添加调试信息
    qDebug() << "执行查询schema语句:" << sql;
    qDebug() << "返回码:" << rc;
    if (errmsg) {
        qDebug() << "错误信息:" << errmsg;
    }
    qDebug() << "获取到的表结构:";
    for (const QStringList &row : result.rows) {
        qDebug() << row;
    }

    if (rc != 0) {
        QString errorMsg = QString("获取表结构失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    QString primaryKeyColumn;
    for (const QStringList &row : result.rows) {
        if (row[8] == "1") { // isPrimaryKey
            primaryKeyColumn = row[1]; // columnName
            break;
        }
    }

    if (primaryKeyColumn.isEmpty()) {
        showError("未找到主键列");
        return;
    }

    sql = QString("DELETE FROM %1 WHERE %2 = '%3'")
        .arg(tableName)
        .arg(primaryKeyColumn)
        .arg(primaryKeyValue);

    rc = GNCDB_exec(db, sql.toUtf8().constData(), nullptr, nullptr, &errmsg);
    
    if (rc != 0) {
        QString errorMsg = QString("删除数据失败: %1").arg(errmsg ? errmsg : "未知错误");
        showError(errorMsg);
        if (errmsg) free(errmsg);
        return;
    }

    // 刷新表格数据
    onTableSelected(tableTree->currentItem(), 0);
}

void MainWindow::onFileAction() {
    // 文件操作的实现
    qDebug() << "文件操作被触发";
}

void MainWindow::onViewAction() {
    // 视图操作的实现
    qDebug() << "视图操作被触发";
}

void MainWindow::onDatabaseAction() {
    // 数据库操作的实现
    qDebug() << "数据库操作被触发";
}

void MainWindow::onSQLAction() {
    // SQL操作的实现
    qDebug() << "SQL操作被触发";
}

void MainWindow::onTransactionAction() {
    // 事务操作的实现
    qDebug() << "事务操作被触发";
}

void MainWindow::onToolsAction() {
    // 工具操作的实现
    qDebug() << "工具操作被触发";
}

void MainWindow::onHelpAction() {
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
