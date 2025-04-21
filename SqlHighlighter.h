#ifndef SQLHIGHLIGHTER_H
#define SQLHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QStringList>

class SqlHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SqlHighlighter(QTextDocument *parent = nullptr);
    ~SqlHighlighter();

    // 设置高亮主题
    void setTheme(const QString &themeName);
    
    // 添加自定义关键字
    void addCustomKeywords(const QStringList &keywords);

protected:
    void highlightBlock(const QString &text) override;

private:
    // 初始化高亮规则
    void setupHighlightingRules();
    
    // 初始化主题
    void setupThemes();
    
    // 高亮规则结构
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    
    // 高亮规则列表
    QVector<HighlightingRule> highlightingRules;
    
    // 关键字列表
    QStringList keywords;
    
    // 自定义关键字列表
    QStringList customKeywords;
    
    // 主题映射
    QMap<QString, QMap<QString, QColor>> themes;
    
    // 当前主题
    QString currentTheme;
};

#endif // SQLHIGHLIGHTER_H 