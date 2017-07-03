#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QSqlQueryModel>
#include <QFile>
#include <Qdir>
#include <QMessageBox>

#define DateTimeFormat "yyyy-MM-dd hh:mm:ss"
#define DatabaseType "QSQLITE"
#define DatabaseName "database.db"
#define DatabaseConnectName "conName"
#define DatabaseTableHtmlList "HtmlList"
#define DatabaseTableUrlList "UrlList"

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    if(QFile(DatabaseName).exists()){
        switch(QMessageBox::question(this,"提示","是否删除数据库")){
        case QMessageBox::Yes:
            qDebug()<<QDir::current().remove(DatabaseName);
            break;
        default:
            break;
        }
    }

    m_NetManger = new QNetworkAccessManager;
    m_NetMangerDetail = new QNetworkAccessManager;

    QObject::connect(m_NetManger, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(NetworkReplyFinishedSlot(QNetworkReply*)));
    QObject::connect(m_NetMangerDetail, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(NetworkDetailReplyFinishedSlot(QNetworkReply*)));

    db = QSqlDatabase::addDatabase(DatabaseType);
    db.setDatabaseName(DatabaseName);
    bool ok = db.open();
    qDebug()<<"db open "<<ok;
    if(!ok){
        QApplication::exit();
    }

    //数据库操作
    QSqlQuery query;
    //一整个网页
    qDebug()<<query.exec("select * from sqlite_master where tbl_name = \'" DatabaseTableHtmlList "\';");
    if(!query.next()){
        QSqlQuery q("create table " DatabaseTableHtmlList " (id integer PRIMARY KEY autoincrement"
                                                          ",dateTime text"
                                                          ",url text"
                                                          ",html text"
                                                          ");"
                    );
    }
    //每条url、对应的name、详细页面
    qDebug()<<query.exec("select * from sqlite_master where tbl_name = \'" DatabaseTableUrlList "\';");
    if(!query.next()){
        QSqlQuery q("create table " DatabaseTableUrlList " (id integer PRIMARY KEY autoincrement"
                                                         ",dateTime text"
                                                         ",getted bool"
                                                         ",url text"
                                                         ",name text"
                                                         ",html text"
                                                         ");"
                    );
    }

    //自动获取下一页
    connect(ui->lineEditUrl,SIGNAL(textChanged(QString)),this,SLOT(on_checkBoxAuto_clicked()));
    connect(ui->lineEditDetail,SIGNAL(textChanged(QString)),this,SLOT(on_checkBoxAutoDetail_clicked()));

    //显示数据库
    modelUrl = new QSqlQueryModel;
    modelHtml = new QSqlQueryModel;
    ui->tableViewPage->setModel(modelHtml);
    ui->tableViewUrl->setModel(modelUrl);

    //具体资源页测试
    ui->tableViewUrl->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewUrl->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui->tableViewUrl->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this,SLOT(tableViewUrlCurrentRowChanged(QModelIndex,QModelIndex)));
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::processListString(const QString &str, const QString &url)
{
    //<a href="/html/gndy/dyzz/20170627/54349.html" class="ulink">2017年动作《金刚：骷髅岛》国英双语.BD中英双字幕</a>
    QRegExp exp("[\\t]<a href=\\\"(.*)\\\".*class=\\\"ulink\\\">(.*)</a>");
    exp.setMinimal(true);

    //寻找并保存资源页url和名字
    int pos = 0;
    while ((pos = exp.indexIn(str, pos)) != -1) {
        QString url = exp. cap(1);
        QString name =  exp. cap(2);

        //插入前检查
        bool isHave=false;

        QSqlQuery query("select * from " DatabaseTableUrlList " where url = '"+url+"'");

        if(query.next()){
            isHave=true;
        }
        qDebug()<<isHave<<" "<<query.lastError().text()<<" "<<query.lastQuery();

        if(isHave&&ui->radioButtonNew->isChecked()){//增量模式，记录被找到
            qDebug()<<"增量模式，且 已找到重复记录";
            return;
        }
        //插入
        if(!isHave){
            QSqlQuery query;
            query.prepare("insert into " DatabaseTableUrlList " values (NULL,?,?,?,?,NUll)");
            query.addBindValue(QDateTime::currentDateTime().toString(DateTimeFormat));
            query.addBindValue(false);
            query.addBindValue("http://www.ygdy8.com"+url);
            query.addBindValue(name);
            dealQueryExec(query,"insert " DatabaseTableUrlList);
        }

        pos += exp.matchedLength();
    }

    //有下一页信息，则继续
    //<a href='list_23_2.html'>下一页</a>
    //http://www.ygdy8.com/html/gndy/dyzz/list_23_2.html
    QString currentPage,nextPage;
    QRegExp expCurrent(".*/");
    QRegExp expNext("^(.*)<a href='(.*)'>下一页</a>");
    expNext.setMinimal(true);
    int posHome = expCurrent.indexIn(url);
    int posNext = expNext.indexIn(str);
    if(posHome>-1&&posNext>-1){
        currentPage = expCurrent.cap(0);
        nextPage = expNext.cap(2);
        qDebug()<<currentPage<<nextPage;
        //m_NetManger->get(QNetworkRequest(QUrl(currentPage+nextPage)));
        ui->lineEditUrl->setText(currentPage+nextPage);
    }else{
        qDebug()<<"homePage:"<<posHome<<" nextPage:"<<nextPage;
    }
}

bool MainWidget::dealQueryExec(QSqlQuery &query, const QString &flag)
{
    if(query.exec()){
        qDebug()<<query.lastQuery();
        return true;
    }else{
        qDebug()<<flag<<" error:"<<query.lastError().text()<<query.lastQuery();
        return false;
    }
}

bool MainWidget::dealQueryExec(const QString &queryString, const QString &flag)
{
    QSqlQuery query(queryString);
    if(query.lastError().isValid()){
        qDebug()<<flag<<" error:"<<query.lastError().text()<<query.lastQuery();
        return false;
    }else{
        return true;
    }
}

void MainWidget::NetworkReplyFinishedSlot(QNetworkReply *m)
{
    QByteArray bytes = m->readAll();
    qDebug()<<"NetworkReplyFinishedSlot:"<<bytes.size()<<" url:"<<m->url().toString();
    QString r= QString::fromLocal8Bit(bytes);
//    ui->textBrowser->setText(r);
//    ui->plainTextEdit->setPlainText(r);

    //保存html网页
    QSqlQuery query;
    query.prepare("insert into " DatabaseTableHtmlList " values (NULL,?,?,?)");
    query.addBindValue(QDateTime::currentDateTime().toString(DateTimeFormat));
    query.addBindValue(m->url().toString());
    query.addBindValue(bytes);
    dealQueryExec(query,"insert " DatabaseTableHtmlList);
    m->deleteLater();
    processListString(r,m->url().toString());
}

void MainWidget::NetworkDetailReplyFinishedSlot(QNetworkReply *m)
{
    QByteArray bytes = m->readAll();
    qDebug()<<"NetworkDetailReplyFinishedSlot:"<<bytes.size()<<" url:"<<m->url().toString();
    QString r= QString::fromLocal8Bit(bytes);

    //保存html网页
    QSqlQuery query;
    query.prepare("update " DatabaseTableUrlList " set getted = 1, html = ? where url = ?");
    query.addBindValue(bytes);
    query.addBindValue(m->url().toString());
    dealQueryExec(query,"update " DatabaseTableUrlList);
    m->deleteLater();

    listDetail.removeAt(0);
    if(listDetail.size()){
        ui->lineEditDetail->setText(listDetail.at(0));
    }else{
        ui->lineEditDetail->setText("完毕");
    }
    ui->plainTextEditHtml->setPlainText(r);
}

void MainWidget::on_pushButtonGet_clicked()
{
    m_NetManger->get(QNetworkRequest(QUrl(ui->lineEditUrl->text())));
    ui->plainTextEdit->appendPlainText(ui->lineEditUrl->text());
}

void MainWidget::on_checkBoxAuto_clicked()
{
    if(ui->checkBoxAuto->isChecked()){
        on_pushButtonGet_clicked();
    }
}

void MainWidget::on_pushButtonUrl_clicked()
{
    modelUrl->setQuery("SELECT * FROM " DatabaseTableUrlList);
}

void MainWidget::on_pushButtonPage_clicked()
{
    modelHtml->setQuery("SELECT * FROM " DatabaseTableHtmlList);
}

void MainWidget::on_pushButtonGetDetail_clicked()
{
    if(listDetail.size()==0){
        QSqlQuery query("select url from " DatabaseTableUrlList " where getted = 0");
        qDebug()<<query.lastError().text()<<"  "<<query.lastQuery();
        while(query.next()){
            listDetail.append(query.value(0).toString());
        }
        if(listDetail.size()>0){
            ui->lineEditDetail->setText(listDetail.at(0));
            on_pushButtonGetDetail_clicked();
        }
    }else{
        QString url = ui->lineEditDetail->text();

        m_NetMangerDetail->get(QNetworkRequest(QUrl(url)));
        ui->plainTextEdit->appendPlainText(url);
    }
}

void MainWidget::on_checkBoxAutoDetail_clicked()
{
    if(ui->checkBoxAutoDetail->isChecked()){
        on_pushButtonGetDetail_clicked();
    }
}

void MainWidget::tableViewUrlCurrentRowChanged(QModelIndex i1, QModelIndex i2)
{
    ui->plainTextEditHtml->clear();
    //ui->plainTextEditHtml->appendPlainText(QString("column %1,row %2").arg(i1.column()).arg(i1.row()));
    //ui->plainTextEditHtml->appendPlainText(QString("column %1,row %2").arg(i2.column()).arg(i2.row()));
    int row = i1.row();
    QByteArray bytes = ui->tableViewUrl->model()->index(row,5).data().toByteArray();
    QString value = QString::fromLocal8Bit(bytes);
    ui->plainTextEditHtml->appendPlainText(value);


}
