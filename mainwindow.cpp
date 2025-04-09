#include "mainwindow.h"
#include "./ui_mainwindow.h"
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

// 定义一个结构体来存储表名
struct TableInfo {
    QString tableName;
};

// 定义一个全局变量来存储表名列表
QVector<TableInfo> tableList;

// C++版本的callback函数
int cppCallback(void *data, int argc, char **azColName, char **argv) {
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
int selectCallback(void *data, int argc, char **azColName, char **argv) {
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
    , db(nullptr)
{
    ui->setupUi(this);
    initUI();
    initConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (db) {
        delete db;
    }
}

void MainWindow::initUI()
{
    // 创建主标签页
    mainTab = new QTabWidget(this);
    setCentralWidget(mainTab);

    // 创建数据库视图标签页
    QWidget *dbView = new QWidget(this);
    QHBoxLayout *dbLayout = new QHBoxLayout(dbView);

    // 左侧表树
    tableTree = new QTreeWidget(this);
    tableTree->setHeaderLabel("数据库表");
    dbLayout->addWidget(tableTree, 1);

    // 右侧数据表格
    dataTable = new QTableWidget(this);
    dbLayout->addWidget(dataTable, 3);

    mainTab->addTab(dbView, "数据库视图");

    // 创建SQL编辑器标签页
    QWidget *sqlView = new QWidget(this);
    QVBoxLayout *sqlLayout = new QVBoxLayout(sqlView);

    sqlEditor = new QTextEdit(this);
    sqlLayout->addWidget(sqlEditor);

    // 添加执行按钮到布局
    sqlLayout->addWidget(ui->executeButton);

    mainTab->addTab(sqlView, "SQL编辑器");

    // 创建工具栏
    QToolBar *toolBar = addToolBar("工具栏");
    toolBar->addAction("连接数据库", this, &MainWindow::onConnectDB);
    toolBar->addAction("断开连接", this, &MainWindow::onDisconnectDB);
    toolBar->addAction("刷新表", this, &MainWindow::onRefreshTables);

    // 创建状态栏
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("未连接数据库");
}

void MainWindow::initConnections()
{
    connect(tableTree, &QTreeWidget::itemClicked, this, &MainWindow::onTableSelected);
    
    // 连接SQL执行按钮的点击事件
    QPushButton *executeButton = ui->executeButton;  // 直接使用UI文件中的按钮
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

    db = new GNCDB();
    int rc;
    
    // 获取项目根目录
    QString projectDir = QDir::current().absolutePath();
    
    // 检查当前路径是否包含 /build/bin，如果是，则返回项目根目录
    if (projectDir.contains("/build/bin")) {
        int buildIndex = projectDir.indexOf("/build");
        if (buildIndex > 0) {
            projectDir = projectDir.left(buildIndex);
        }
    } else if (projectDir.endsWith("/build")) {
        projectDir = projectDir.left(projectDir.length() - 6); // 移除 "/build"
    }
    
    qDebug() << "项目根目录：" << projectDir;
    QString dbPath = QDir(projectDir).absoluteFilePath("testdb.db");
    qDebug() << "尝试打开数据库文件：" << dbPath;
    
    // 将 QString 转换为 char* 类型
    QByteArray dbPathBytes = dbPath.toLocal8Bit();
    char* dbPathChar = dbPathBytes.data();
    
    if ((rc = GNCDB_open(&db, dbPathChar)) == 0) {
        statusBar->showMessage("数据库连接成功");
        updateTableList();
    } else {
        QString errorMsg = QString("数据库连接失败，错误代码: %1，文件路径: %2").arg(rc).arg(dbPath);
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

    GNCDB_close(&db); // 确保函数调用正确
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
    
    // 清空输入框
    sqlEditor->clear();
    
    // 创建SQL结果对象
    SQLResult result;
    
    // 执行SQL语句
    char *errmsg = nullptr;
    qDebug() << "调用GNCDB_exec...";
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    qDebug() << "GNCDB_exec返回码:" << rc;
    
    if (rc != 0) {
        QString errorMsg = QString("SQL执行失败: %1").arg(errmsg ? errmsg : "未知错误");
        qDebug() << errorMsg;
        showError(errorMsg);
        if (errmsg) {
            free(errmsg);
        }
        return;
    }
    
    qDebug() << "SQL执行成功，结果行数:" << result.rows.size();
    qDebug() << "列名:" << result.columnNames;
    
    // 显示结果
    displaySQLResult(result);
}

void MainWindow::displaySQLResult(const SQLResult &result)
{
    qDebug() << "开始显示SQL结果...";
    
    // 清空数据表格
    dataTable->clear();
    
    // 设置列数和列标题
    dataTable->setColumnCount(result.columnNames.size());
    dataTable->setHorizontalHeaderLabels(result.columnNames);
    qDebug() << "设置列数:" << result.columnNames.size();
    
    // 设置行数
    dataTable->setRowCount(result.rows.size());
    qDebug() << "设置行数:" << result.rows.size();
    
    // 填充数据
    for (int row = 0; row < result.rows.size(); row++) {
        const QStringList &rowData = result.rows[row];
        qDebug() << "处理第" << row << "行数据:" << rowData;
        for (int col = 0; col < rowData.size(); col++) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[col]);
            dataTable->setItem(row, col, item);
        }
    }
    
    // 调整列宽
    dataTable->resizeColumnsToContents();
    
    // 显示结果数量
    QString statusMsg = QString("查询完成，返回 %1 行数据").arg(result.rows.size());
    qDebug() << statusMsg;
    statusBar->showMessage(statusMsg);
    
    qDebug() << "SQL结果显示完成";
}

void MainWindow::onShowTables()
{
    if (!db) {
        showError("请先连接数据库");
        return;
    }
    updateTableList();
}

void MainWindow::onTableSelected(QTreeWidgetItem *item, int column)
{
    if (!db || !item) return;
    
    QString tableName = item->text(0);
    qDebug() << "选中表:" << tableName;
    
    // 创建SQL结果对象
    SQLResult result;
    
    // 构建SELECT语句
    QString sql = QString("SELECT * FROM %1").arg(tableName);
    qDebug() << "执行SQL:" << sql;
    
    // 执行SQL语句
    char *errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), sqlResultCallback, &result, &errmsg);
    
    if (rc != 0) {
        QString errorMsg = QString("查询表 %1 失败: %2").arg(tableName).arg(errmsg ? errmsg : "未知错误");
        qDebug() << errorMsg;
        showError(errorMsg);
        if (errmsg) {
            free(errmsg);
        }
        return;
    }
    
    qDebug() << "查询成功，结果行数:" << result.rows.size();
    
    // 显示结果
    displaySQLResult(result);
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

    qDebug() << "开始更新表列表";
    tableTree->clear();
    
    // 清空表名列表
    tableList.clear();
    
    // 使用GNCDB_select函数获取所有表名
    int affectedRows = 0;
    qDebug() << "执行SQL查询: SELECT tableName FROM master;";
    //int rc = GNCDB_select(db, selectCallback, &affectedRows, &tableList, 1, 1, 0);
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
        QTreeWidgetItem *item = new QTreeWidgetItem(tableTree);
        item->setText(0, table.tableName);
        qDebug() << "添加表到树形控件:" << table.tableName;
    }
    
    // 展开所有项
    tableTree->expandAll();
    qDebug() << "表列表更新完成";
}

void MainWindow::showTableData(const QString &tableName)
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
