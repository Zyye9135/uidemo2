#include "SqlHighlighter.h"
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <QElapsedTimer>

SqlHighlighter::SqlHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , currentTheme("Light")
{
    // 确保在主线程中创建
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    
    // 初始化SQL关键字
    keywords = {
        "SELECT", "FROM", "WHERE", "GROUP BY", "ORDER BY", "HAVING", "JOIN", "LEFT", "RIGHT", "INNER", "OUTER",
        "ON", "AS", "AND", "OR", "NOT", "IN", "EXISTS", "LIKE", "IS", "NULL", "ISNULL", "NOTNULL",
        "INSERT", "INTO", "VALUES", "UPDATE", "SET", "DELETE", "CREATE", "TABLE", "ALTER", "DROP",
        "INDEX", "VIEW", "TRIGGER", "PROCEDURE", "FUNCTION", "BEGIN", "END", "IF", "ELSE", "THEN",
        "CASE", "WHEN", "ELSE", "END", "LOOP", "WHILE", "FOR", "EXIT", "CONTINUE", "RETURN",
        "COMMIT", "ROLLBACK", "SAVEPOINT", "GRANT", "REVOKE", "DENY", "UNION", "ALL", "DISTINCT",
        "COUNT", "SUM", "AVG", "MIN", "MAX", "TOP", "LIMIT", "OFFSET", "ASC", "DESC"
    };
    
    // 初始化主题
    setupThemes();
    
    // 设置高亮规则
    setupHighlightingRules();
    
    qDebug() << "高亮器初始化完成，主题数:" << themes.size();
}

SqlHighlighter::~SqlHighlighter()
{
}

void SqlHighlighter::setTheme(const QString &themeName)
{
    if (themes.contains(themeName)) {
        currentTheme = themeName;
        setupHighlightingRules();
        rehighlight();
    } else {
        qDebug() << "主题" << themeName << "不存在，使用默认主题";
    }
}

void SqlHighlighter::addCustomKeywords(const QStringList &newKeywords)
{
    customKeywords = newKeywords;
    setupHighlightingRules();
    rehighlight();
}

void SqlHighlighter::highlightBlock(const QString &text)
{
    // 确保在主线程
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    
    // 添加空文本检查
    if (text.isEmpty()) {
        return;
    }
    
    // 限制超长文本处理
    if (text.length() > 1000) {
        return;
    }
    
    try {
        // 应用所有高亮规则
        for (const HighlightingRule &rule : highlightingRules) {
            if (!rule.pattern.isValid()) {
                continue;
            }
            
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                if (match.hasMatch()) {
                    setFormat(match.capturedStart(), match.capturedLength(), rule.format);
                }
            }
        }
        
        // 高亮字符串（添加转义字符支持）
        static QRegularExpression stringPattern(R"('(?:\\.|[^'\\])*'|"(?:\\.|[^"\\])*")");
        if (stringPattern.isValid()) {
            QRegularExpressionMatchIterator stringMatchIterator = stringPattern.globalMatch(text);
            QTextCharFormat stringFormat;
            stringFormat.setForeground(themes.value(currentTheme).value("string", Qt::black));
            while (stringMatchIterator.hasNext()) {
                QRegularExpressionMatch match = stringMatchIterator.next();
                if (match.hasMatch()) {
                    setFormat(match.capturedStart(), match.capturedLength(), stringFormat);
                }
            }
        }
        
        // 高亮注释
        static QRegularExpression commentPattern("--.*$|/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/");
        if (commentPattern.isValid()) {
            QRegularExpressionMatchIterator commentMatchIterator = commentPattern.globalMatch(text);
            QTextCharFormat commentFormat;
            commentFormat.setForeground(themes.value(currentTheme).value("comment", Qt::black));
            while (commentMatchIterator.hasNext()) {
                QRegularExpressionMatch match = commentMatchIterator.next();
                if (match.hasMatch()) {
                    setFormat(match.capturedStart(), match.capturedLength(), commentFormat);
                }
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "高亮处理异常:" << e.what();
    } catch (...) {
        qCritical() << "未知高亮异常";
    }
}

void SqlHighlighter::setupHighlightingRules()
{
    highlightingRules.clear();
    
    auto addRule = [this](const QString &pattern, const QColor &color, bool bold = false) {
        QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) {
            qWarning() << "忽略无效模式:" << pattern << "错误:" << re.errorString();
            return;
        }
        
        QTextCharFormat format;
        format.setForeground(color);
        if (bold) format.setFontWeight(QFont::Bold);
        
        highlightingRules.append({re, format});
    };
    
    // 添加SQL关键字规则
    for (const QString &keyword : keywords) {
        addRule("\\b" + QRegularExpression::escape(keyword) + "\\b", 
               themes.value(currentTheme).value("keyword", Qt::black), true);
    }
    
    // 添加自定义关键字规则
    for (const QString &keyword : customKeywords) {
        addRule("\\b" + QRegularExpression::escape(keyword) + "\\b", 
               themes.value(currentTheme).value("custom", Qt::black), true);
    }
    
    // 函数高亮
    addRule("\\b[A-Za-z0-9_]+(?=\\()", 
           themes.value(currentTheme).value("function", Qt::black), false);
    
    // 数字高亮
    addRule("\\b\\d+\\b", 
           themes.value(currentTheme).value("number", Qt::black), false);
}

void SqlHighlighter::setupThemes()
{
    // 浅色主题
    QMap<QString, QColor> lightTheme;
    lightTheme["keyword"] = QColor(0, 0, 255);      // 蓝色
    lightTheme["function"] = QColor(128, 0, 128);   // 紫色
    lightTheme["string"] = QColor(0, 128, 0);       // 绿色
    lightTheme["comment"] = QColor(128, 128, 128);  // 灰色
    lightTheme["number"] = QColor(255, 0, 0);       // 红色
    lightTheme["custom"] = QColor(0, 128, 128);     // 青色
    
    // 深色主题
    QMap<QString, QColor> darkTheme;
    darkTheme["keyword"] = QColor(0, 191, 255);     // 浅蓝色
    darkTheme["function"] = QColor(255, 128, 255);  // 粉色
    darkTheme["string"] = QColor(144, 238, 144);    // 浅绿色
    darkTheme["comment"] = QColor(192, 192, 192);   // 银色
    darkTheme["number"] = QColor(255, 128, 128);    // 浅红色
    darkTheme["custom"] = QColor(0, 255, 255);      // 黄色
    
    // 添加主题
    themes["Light"] = lightTheme;
    themes["Dark"] = darkTheme;
} 