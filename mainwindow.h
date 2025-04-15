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
#include "gncdblib/include/gncdb.h"
#include "database/dbmanager.h"
#include "database/sqlresult.h"

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

private slots:
    void onConnectDB();
    void onDisconnectDB();
    void onExecuteSQL();
    void onShowTables();
    void onTableSelected(QTreeWidgetItem *item, int column);
    void onRefreshTables();
    void onCreateTable();
    void onDropTable();
    void onAddRow();
    void onEditRow();
    void onDeleteRow();
    void showTableContextMenu(const QPoint &pos);
    void showCreateTableDialog();

private:
    Ui::MainWindow *ui;
    DBManager *dbManager;
    GNCDB *db;
    QTreeWidget *tableTree;
    QTableWidget *dataTable;
    QTextEdit *sqlEditor;
    QTabWidget *mainTab;
    QStatusBar *statusBar;
    QString currentTable;
    QStringList currentColumnNames;

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
    FieldType getFieldTypeFromString(const QString &typeStr);
    QString getFieldTypeName(int typeId);
    void loadTableColumns(QTreeWidgetItem *tableItem);
};

#endif // MAINWINDOW_H
