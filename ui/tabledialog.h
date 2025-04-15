#ifndef TABLEDIALOG_H
#define TABLEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QList>

class TableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TableDialog(QWidget *parent = nullptr);
    ~TableDialog();

    QString getTableName() const;
    QList<QString> getColumnNames() const;
    QList<QString> getColumnTypes() const;
    QList<int> getColumnLengths() const;

private:
    void setupUI();
    void addColumnControls();

    QLineEdit *tableNameEdit;
    QWidget *columnsWidget;
    QVBoxLayout *columnsLayout;
    QList<QHBoxLayout*> columnLayouts;
    QList<QLineEdit*> columnNames;
    QList<QComboBox*> columnTypes;
    QList<QSpinBox*> columnLengths;
};

#endif // TABLEDIALOG_H
