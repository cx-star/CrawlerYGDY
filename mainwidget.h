#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QtNetwork>
#include <QSql>
#include <QSqlDatabase>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

private:
    Ui::MainWidget *ui;
    QNetworkAccessManager *m_NetManger;
    QNetworkReply *m_Reply;
    void processListString(const QString& str,const QString& url);//处理内容列表的网页
    QSqlDatabase db;//用于存储从网页服务器获取的网页
    bool dealQueryExec(QSqlQuery& query, const QString& flag);
    bool dealQueryExec(const QString& queryString, const QString& flag);
private slots:
    void NetworkReplyFinishedSlot(QNetworkReply*m);
    void on_pushButtonGet_clicked();
    void on_checkBoxAuto_clicked();
};

#endif // MAINWIDGET_H
