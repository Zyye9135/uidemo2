#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QTabWidget>
#include <QStatusBar>
#include <QStringList>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDebug>
#include <QVector>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QCloseEvent>
#include "gncdblib/include/gncdb.h"
#include "database/dbmanager.h"
#include "database/sqlresult.h"
#include "SqlHighlighter.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onConnectDB();
    void onDisconnectDB();
    void onExecuteSQL();
    void onExecuteCurrentLine();
    void onFormatSQL();
    void onOpenSQLScript();
    void onSaveSQLScript();
    void onSaveAsSQLScript();
    void onShowTables();
    void onTableSelected(QTreeWidgetItem *item, int column);
    void onRefreshTables();
    void onCreateTable();
    void onDropTable();
    void onAddRow();
    void onEditRow();
    void onDeleteRow();
    //void onRenameTable();  // 新增：重命名表
    void onClearTable();   // 清空表
    void showTableContextMenu(const QPoint &pos);
    void showCreateTableDialog();
    void onFileAction();       // 文件操作
    void onViewAction();       // 视图操作
    void onDatabaseAction();   // 数据库操作
    void onSQLAction();        // SQL操作
    void onTransactionAction(); // 事务操作
    void onToolsAction();      // 工具操作
    void onHelpAction();       // 帮助操作
    void onObjectAction();     // 对象操作
    void onTableManagementAction(); // 表管理操作
    void onNewDatabase();      // 新建数据库
    void onExit();             // 退出程序
    void updateDDLView(const QString &tableName);  // 更新DDL视图
    
    // 新增视图菜单相关的槽函数
    void onShowTreeView();     // 显示树形图
    void onShowDatabaseTab();  // 显示数据库标签
    void onShowDataTab();      // 显示数据标签
    void onShowDDLTab();       // 显示DDL标签
    void onShowSQLTab();       // 显示SQL标签
    void onZoomIn();
    void onZoomOut();

private:
    Ui::MainWindow *ui;
    DBManager *dbManager;
    GNCDB *db;
    
    // 布局组件
    QSplitter *mainSplitter;
    QTreeWidget *tableTree;
    QTabWidget *rightTabWidget;
    QTableWidget *dataTable;
    QTextEdit *sqlEditor;
    QWidget *databaseTab;
    QStatusBar *statusBar;
    QTextEdit *ddlEditor;  // DDL编辑器
    
    // 菜单相关
    QMenuBar *menuBar;
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *databaseMenu;
    QMenu *tableMenu;
    QMenu *viewMenu;
    QMenu *sqlMenu;
    QMenu *transMenu;
    QMenu *toolsMenu;
    QMenu *helpMenu;
    
    // 工具栏相关
    QToolBar *mainToolBar;
    QToolButton *chartButton;
    QMenu *chartMenu;
    
    // 当前状态
    QString currentTable;
    QStringList currentColumnNames;
    QString currentSQLFile;  // 当前打开的SQL文件路径

    QTableWidget *sqlResultDisplay;  // SQL结果显示表格
    
    // SQL高亮器
    SqlHighlighter *sqlHighlighter;

    void initUI();
    void initConnections();
    void updateTableList();
    void showTableData(const QString &tableName);
    void showError(const QString &message);
    void setupTableManagementActions();
    void showEditRowDialog(const QStringList &rowData = QStringList());
    void executeInsertSQL(const QString &tableName, const QStringList &values);
    void executeUpdateSQL(const QString &tableName, const QStringList &values, int rowIndex, const QMap<int, FieldType> &columnFieldTypes);
    void executeDeleteSQL(const QString &tableName, int rowIndex);
    void executeSQLStatement(const QString &sql);  // 执行SQL语句的辅助函数
    FieldType getFieldTypeFromString(const QString &typeStr);
    QString getFieldTypeName(int typeId);
    void loadTableColumns(QTreeWidgetItem *tableItem);
};

#endif // MAINWINDOW_H
