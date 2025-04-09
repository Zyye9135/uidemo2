#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>  // access() 用于检查文件读写权限
#include "db.h"

// 定义一个全局配置变量，控制是否打印列名
int printColumnNames = 1; // 默认为1，表示打印列名
int headerPrinted = 0;
int ifExcel = 0;  // 控制是否以电子表格格式输出（CSV）
int columnWidth = 15; // 默认列宽为15
FILE *outputFile = NULL; // 文件指针，初始化为NULL
extern char *colSeparator;  // 列分隔符
extern char *rowSeparator;  // 行分隔符

// 提供一个接口函数用于修改配置
void set_print_column_names(int print) {
    printColumnNames = print;
}

// 控制输出是否为电子表格格式（CSV）
void set_output_excel(int enable) {
    ifExcel = enable;  // 启用或禁用电子表格格式输出
}

// 设置列宽
void set_column_width(int width) {
    columnWidth = width;
}

// 打开输出文件
int open_output_file(const char *fileName) {
    // 检查文件是否存在，如果不存在则创建文件
    outputFile = fopen(fileName, "w"); // 以写入模式打开文件，如果文件不存在会创建
    if (outputFile == NULL) {
        perror("Error opening file");
        return -1;
    }
    printf("File opened/created successfully: %s\n", fileName);
    return 0;
}

// 写入输出文件
void write_to_file(const char *content) {
    if (outputFile != NULL) {
        fprintf(outputFile, "%s", content);
    }
    else {
        printf("Error: No output file opened.\n");
    }
}

// 关闭输出文件
void close_output_file() {
    if (outputFile != NULL) {
        fclose(outputFile);
        printf("File closed successfully.\n");
        outputFile = NULL;
    }
    else {
        printf("Error: No output file to close.\n");
    }
}

// 标准输出的打印
int callback(void *data, int argc, char **azColName, char **argv) {
    // 如果需要以电子表格格式输出
    if (ifExcel) {
        if (headerPrinted == 0) {
            // 打印列名
            for (int i = 0; i < argc; i++) {
                printf("\"%s\"", azColName[i]);
                if (i < argc - 1) {
                    printf("%s", colSeparator);  // 用列分隔符分隔列名
                }
            }
            printf("%s", rowSeparator);  // 列名后换行
            headerPrinted = 1;

            // 如果文件输出已启用，将列名写入文件
            if (outputFile != NULL) {
                for (int i = 0; i < argc; i++) {
                    fprintf(outputFile, "\"%s\"", azColName[i]);
                    if (i < argc - 1) {
                        fprintf(outputFile, "%s", colSeparator);
                    }
                }
                fprintf(outputFile, "%s", rowSeparator);
            }
        }

        // 打印数据
        for (int i = 0; i < argc; i++) {
            printf("\"%s\"", argv[i] ? argv[i] : "NULL");
            if (i < argc - 1) {
                printf("%s", colSeparator);  // 用列分隔符分隔列数据
            }
        }
        printf("%s", rowSeparator);

        // 如果文件输出已启用，将数据写入文件
        if (outputFile != NULL) {
            for (int i = 0; i < argc; i++) {
                fprintf(outputFile, "\"%s\"", argv[i] ? argv[i] : "NULL");
                if (i < argc - 1) {
                    fprintf(outputFile, "%s", colSeparator);
                }
            }
            fprintf(outputFile, "%s", rowSeparator);
        }
    }
    else {
        // 普通输出（不为电子表格格式）
        if (printColumnNames && !headerPrinted) {
            for (int i = 0; i < argc; i++) {
                if (strcmp(azColName[i], "createTime") == 0 ||
                    strcmp(azColName[i], "updateTime") == 0 ||
                    strcmp(azColName[i], "rowId") == 0) {
                    continue;
                }
                printf("%-*s%s", columnWidth, azColName[i], colSeparator);
            }
            printf("%s", rowSeparator);

            for (int i = 0; i < argc; i++) {
                if (strcmp(azColName[i], "createTime") == 0 ||
                    strcmp(azColName[i], "updateTime") == 0 ||
                    strcmp(azColName[i], "rowId") == 0) {
                    continue;
                }
                printf("%-*s%s", columnWidth, "------", colSeparator);
            }
            printf("%s", rowSeparator);

            headerPrinted = 1;
        }

        for (int i = 0; i < argc; i++) {
            if (strcmp(azColName[i], "createTime") == 0 ||
                strcmp(azColName[i], "updateTime") == 0 ||
                strcmp(azColName[i], "rowId") == 0) {
                continue;
            }
            printf("%-*s%s", columnWidth, argv[i] ? argv[i] : "NULL", colSeparator);
        }
        printf("%s", rowSeparator);

        // 如果文件输出已启用，将数据写入文件
        if (outputFile != NULL) {
            for (int i = 0; i < argc; i++) {
                fprintf(outputFile, "%-*s%s", columnWidth, argv[i] ? argv[i] : "NULL", colSeparator);
            }
            fprintf(outputFile, "%s", rowSeparator);
        }
    }
    return 0;
}

// callback_echo_off 函数：打印列名和数据行（不回显数据）
int callback_echo_off(void *data, int argc, char **azColName, char **argv) {
    for(int i = 0; i < argc; i++) {
        if(strcmp(azColName[i], "UPDATE COUNT") == 0 ||
           strcmp(azColName[i], "DELETE COUNT") == 0 ||
           strcmp(azColName[i], "DELETE COUNT") == 0) {
           return 0;
        }
    }
    if (ifExcel) {
        // 以电子表格格式输出（CSV）
        if (headerPrinted == 0) {
            // 打印列名
            for (int i = 0; i < argc; i++) {
                printf("\"%s\"", azColName[i]);
                if (i < argc - 1) {
                    printf("%s", colSeparator);  // 用列分隔符分隔列名
                }
            }
            printf("%s", rowSeparator);  // 列名后换行
            headerPrinted = 1;

            // 如果文件输出已启用，将列名写入文件
            if (outputFile != NULL) {
                for (int i = 0; i < argc; i++) {
                    fprintf(outputFile, "\"%s\"", azColName[i]);
                    if (i < argc - 1) {
                        fprintf(outputFile, "%s", colSeparator);
                    }
                }
                fprintf(outputFile, "%s", rowSeparator);
            }
        }

        // 打印数据
        for (int i = 0; i < argc; i++) {
            printf("\"%s\"", argv[i] ? argv[i] : "NULL");
            if (i < argc - 1) {
                printf("%s", colSeparator);  // 用列分隔符分隔列数据
            }
        }
        printf("%s", rowSeparator);

        // 如果文件输出已启用，将数据写入文件
        if (outputFile != NULL) {
            for (int i = 0; i < argc; i++) {
                fprintf(outputFile, "\"%s\"", argv[i] ? argv[i] : "NULL");
                if (i < argc - 1) {
                    fprintf(outputFile, "%s", colSeparator);
                }
            }
            fprintf(outputFile, "%s", rowSeparator);
        }
    }
    else {
        // 普通输出（不为电子表格格式）
        if (printColumnNames && !headerPrinted) {
            for (int i = 0; i < argc; i++) {
                
                if (strcmp(azColName[i], "createTime") == 0 ||
                    strcmp(azColName[i], "updateTime") == 0 ||
                    strcmp(azColName[i], "rowId") == 0) {
                    continue;
                }
                printf("%-*s%s", columnWidth, azColName[i], colSeparator);
            }
            printf("%s", rowSeparator);

            for (int i = 0; i < argc; i++) {
                if (strcmp(azColName[i], "createTime") == 0 ||
                    strcmp(azColName[i], "updateTime") == 0 ||
                    strcmp(azColName[i], "rowId") == 0) {
                    continue;
                }
                printf("%-*s%s", columnWidth, "------", colSeparator);
            }
            printf("%s", rowSeparator);

            headerPrinted = 1;
        }

        for (int i = 0; i < argc; i++) {
            if (strcmp(azColName[i], "createTime") == 0 ||
                strcmp(azColName[i], "updateTime") == 0 ||
                strcmp(azColName[i], "rowId") == 0) {
                continue;
            }
            printf("%-*s%s", columnWidth, argv[i] ? argv[i] : "NULL", colSeparator);
        }
        printf("%s", rowSeparator);

        // 如果文件输出已启用，将数据写入文件
        if (outputFile != NULL) {
            for (int i = 0; i < argc; i++) {
                fprintf(outputFile, "%-*s%s", columnWidth, argv[i] ? argv[i] : "NULL", colSeparator);
            }
            fprintf(outputFile, "%s", rowSeparator);
        }
    }
    return 0;
}