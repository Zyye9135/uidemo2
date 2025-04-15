#include "tabledialog.h"

TableDialog::TableDialog(QWidget *parent)
    : QDialog(parent)
    , tableNameEdit(nullptr)
    , columnsWidget(nullptr)
    , columnsLayout(nullptr)
{
    setupUI();
}

TableDialog::~TableDialog()
{
}

void TableDialog::setupUI()
{
    setWindowTitle(tr("新建表"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 表名输入
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel(tr("表名:"), this);
    tableNameEdit = new QLineEdit(this);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(tableNameEdit);
    mainLayout->addLayout(nameLayout);

    // 列定义区域
    columnsWidget = new QWidget(this);
    columnsLayout = new QVBoxLayout(columnsWidget);
    mainLayout->addWidget(columnsWidget);

    // 添加第一列
    addColumnControls();

    // 添加列按钮
    QPushButton *addColumnBtn = new QPushButton(tr("添加列"), this);
    connect(addColumnBtn, &QPushButton::clicked, this, &TableDialog::addColumnControls);
    mainLayout->addWidget(addColumnBtn);

    // 确定和取消按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void TableDialog::addColumnControls()
{
    QHBoxLayout *columnLayout = new QHBoxLayout();
    
    QLineEdit *colName = new QLineEdit(this);
    colName->setPlaceholderText(tr("列名"));
    
    QComboBox *colType = new QComboBox(this);
    colType->addItems({tr("INTEGER"), tr("REAL"), tr("VARCHAR"), 
                      tr("TEXT"), tr("BLOB"), tr("DATE"), tr("DATETIME")});
    
    QSpinBox *colLength = new QSpinBox(this);
    colLength->setRange(1, 255);
    colLength->setValue(32);
    colLength->setEnabled(false);

    // 当类型改变时更新长度输入框的状态
    connect(colType, &QComboBox::currentTextChanged, 
            [colLength](const QString &text) {
        colLength->setEnabled(text == "VARCHAR");
    });

    columnLayout->addWidget(colName);
    columnLayout->addWidget(colType);
    columnLayout->addWidget(colLength);

    columnNames.append(colName);
    columnTypes.append(colType);
    columnLengths.append(colLength);
    columnLayouts.append(columnLayout);
    columnsLayout->addLayout(columnLayout);
}

QString TableDialog::getTableName() const
{
    return tableNameEdit->text().trimmed();
}

QList<QString> TableDialog::getColumnNames() const
{
    QList<QString> names;
    for (QLineEdit *edit : columnNames) {
        names.append(edit->text().trimmed());
    }
    return names;
}

QList<QString> TableDialog::getColumnTypes() const
{
    QList<QString> types;
    for (QComboBox *combo : columnTypes) {
        types.append(combo->currentText());
    }
    return types;
}

QList<int> TableDialog::getColumnLengths() const
{
    QList<int> lengths;
    for (QSpinBox *spin : columnLengths) {
        lengths.append(spin->value());
    }
    return lengths;
}