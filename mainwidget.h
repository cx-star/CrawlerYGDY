#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QtNetwork>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQueryModel>

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
    void debugMsg(const QString& s);
    QNetworkAccessManager *m_NetManger,*m_NetMangerDetail;
    QNetworkReply *m_Reply;
    void processListString(const QString& str,const QString& url);//处理内容列表的网页
    QSqlDatabase db;//用于存储从网页服务器获取的网页
    bool dealQueryExec(QSqlQuery& query, const QString& flag);
    bool dealQueryExec(const QString& queryString, const QString& flag);
    QSqlQueryModel *modelUrl;
    QSqlQueryModel *modelHtml;
    QList<QString> listDetail;
private slots:
    void NetworkReplyFinishedSlot(QNetworkReply*m);
    void NetworkDetailReplyFinishedSlot(QNetworkReply*m);
    void on_pushButtonGet_clicked();
    void on_checkBoxAuto_clicked();
    void on_pushButtonUrl_clicked();
    void on_pushButtonPage_clicked();
    void on_pushButtonGetDetail_clicked();
    void on_checkBoxAutoDetail_clicked();
    void tableViewUrlCurrentRowChanged(QModelIndex i1, QModelIndex i2);
};

#endif // MAINWIDGET_H
