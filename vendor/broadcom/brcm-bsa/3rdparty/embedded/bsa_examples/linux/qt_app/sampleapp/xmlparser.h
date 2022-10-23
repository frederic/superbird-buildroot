#ifndef XMLPARSER_H
#define XMLPARSER_H

#include <QXmlStreamReader>
#include <QListWidget>
#include <QComboBox>
#include "bsa.h"

class PBAPListXmlStreamReader
{
public:
    PBAPListXmlStreamReader(QListWidget *tree);

    bool readFile(const QString &fileName);

private:
    void readVCardListingElement();
    void readCardElement();
    void readPageElement();
    void skipUnknownElement();

    QListWidget *pListWidget;
    QXmlStreamReader reader;
};


class MCEFolderListXmlParser
{
public:
    MCEFolderListXmlParser(QListWidget *tree);
    MCEFolderListXmlParser(QComboBox *tree);

    bool readFile(const QString &fileName);

private:
    void readFolderListingElement();
    void readFolderElement();

    QListWidget *pListWidget;
    QComboBox *pComboBox;
    QXmlStreamReader reader;
};

class MCEMessageListXmlParser
{
public:
    MCEMessageListXmlParser(QListWidget *tree, QList<tBSA_MSG_LIST_ITEM>* pMsgList);

    bool readFile(const QString &fileName);

private:
    void readMsgListingElement();
    void readMsgElement();

    QListWidget *pListWidget;
    QXmlStreamReader reader;
    QList<tBSA_MSG_LIST_ITEM>* pMsgListing;
};

class MCEEventReportXmlParser
{
public:
    MCEEventReportXmlParser(QListWidget *tree);

    bool readFile(const QString &fileName);

private:
    void readEventReportElement();
    void readEventElement();

    QListWidget *pListWidget;
    QXmlStreamReader reader;
};

#endif // XMLPARSER_H
