#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QSqlQuery>
#include <QSqlError>
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

    m_NetManger = new QNetworkAccessManager;

    QObject::connect(m_NetManger, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(NetworkReplyFinishedSlot(QNetworkReply*)));

    db = QSqlDatabase::addDatabase(DatabaseType);
    db.setDatabaseName(DatabaseName);
    bool ok = db.open();
    qDebug()<<"db open "<<ok;
    if(!ok){
        QApplication::exit();
    }

    QSqlQuery query;
    qDebug()<<query.exec("select * from sqlite_master where tbl_name = \'" DatabaseTableHtmlList "\';");
    if(!query.next()){
        QSqlQuery q("create table " DatabaseTableHtmlList " (id integer PRIMARY KEY autoincrement"
                                                          ",url text"
                                                          ",html text"
                                                          ");"
                    );
    }
    qDebug()<<query.exec("select * from sqlite_master where tbl_name = \'" DatabaseTableUrlList "\';");
    if(!query.next()){
        QSqlQuery q("create table " DatabaseTableUrlList " (id integer PRIMARY KEY autoincrement"
                                                          ",url text"
                                                          ",name text"
                                                          ");"
                    );
    }

    //自动获取下一页
    connect(ui->lineEditUrl,SIGNAL(textChanged(QString)),this,SLOT(on_checkBoxAuto_clicked()));
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
            query.prepare("insert into " DatabaseTableUrlList " values (NULL,?,?)");
            query.addBindValue(url);
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
    query.prepare("insert into " DatabaseTableHtmlList " values (NULL,?,?)");
    query.addBindValue(m->url().toString());
    query.addBindValue(bytes);
    dealQueryExec(query,"insert " DatabaseTableHtmlList);
    m->deleteLater();
    processListString(r,m->url().toString());
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
