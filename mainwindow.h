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
#include "gncdblib/include/gncdb.h"

// 定义一个结构体来存储SQL查询结果
struct SQLResult {
    QStringList columnNames;
    QVector<QStringList> rows;
};

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
    void displaySQLResult(const SQLResult &result);
private:
    Ui::MainWindow *ui;
    GNCDB *db;
    QTreeWidget *tableTree;
    QTableWidget *dataTable;
    QTextEdit *sqlEditor;
    QTabWidget *mainTab;
    QStatusBar *statusBar;

    void initUI();
    void initConnections();
    void updateTableList();
    void showTableData(const QString &tableName);
    void showError(const QString &message);
};
#endif // MAINWINDOW_H
