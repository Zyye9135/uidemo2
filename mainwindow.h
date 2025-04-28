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
#include <QStyledItemDelegate>
#include <QGroupBox>
#include "gncdblib/include/gncdb.h"
#include "database/dbmanager.h"
#include "database/sqlresult.h"
#include "SqlHighlighter.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class SQLTableDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    SQLTableDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        qDebug() << "创建编辑器";
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    
    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        qDebug() << "设置编辑器数据";
        QStyledItemDelegate::setEditorData(editor, index);
    }
    
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
        qDebug() << "设置模型数据";
        QString oldValue = model->data(index, Qt::DisplayRole).toString();
        QStyledItemDelegate::setModelData(editor, model, index);
        QString newValue = model->data(index, Qt::DisplayRole).toString();
        
        if (oldValue != newValue) {
            qDebug() << "值已改变，触发更新";
            emit const_cast<SQLTableDelegate*>(this)->dataChanged(index, newValue, oldValue);
        }
    }
    
signals:
    void dataChanged(const QModelIndex &index, const QString &newValue, const QString &oldValue);
};

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
    void onClearTable();
    void showTableContextMenu(const QPoint &pos);
    void showCreateTableDialog();
    void onFileAction();
    void onViewAction();
    void onDatabaseAction();
    void onSQLAction();
    void onTransactionAction();
    void onToolsAction();
    void onHelpAction();
    void onObjectAction();
    void onTableManagementAction();
    void onNewDatabase();
    void onExit();
    void updateDDLView(const QString &tableName);
    void onShowTreeView();
    void onShowDatabaseTab();
    void onShowDataTab();
    void onShowDDLTab();
    void onShowSQLTab();
    void onZoomIn();
    void onZoomOut();
    void onEditSQLResult();
    void onCellEdited(QTableWidgetItem *item);
    void onSearchTable();    // 表查找
    void onSearchColumn();   // 段查找
    void onSearchValue();    // 数值查找
    void updateDatabaseInfo(); // 更新数据库信息
    void onVacuumDatabase(); // 压缩数据库

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
    QLabel *dbTitleLabel;  // 数据库标题标签
    
    // 菜单相关
    QMenuBar *menuBar;
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *databaseMenu;
    QMenu *tableMenu;
    QMenu *viewMenu;
    QMenu *sqlMenu;
    QMenu *searchMenu;
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

    // 状态栏标签
    QLabel *dbNameLabel;
    QLabel *tableNameLabel;
    QLabel *dbPathLabel;

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

// 添加TreeItemDelegate类声明
class TreeItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit TreeItemDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    QIcon databaseIcon;
    QIcon tableIcon;
    QIcon columnIcon;
};

#endif // MAINWINDOW_H
