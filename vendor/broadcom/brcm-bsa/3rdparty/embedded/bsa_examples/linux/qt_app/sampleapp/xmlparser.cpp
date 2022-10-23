#include <QFile>
#include <iostream>
#include "xmlparser.h"
#include <QXmlStreamAttributes>

using namespace std;

PBAPListXmlStreamReader::PBAPListXmlStreamReader(QListWidget *pList)
{
    pListWidget = pList;
    pListWidget->clear();
}

bool PBAPListXmlStreamReader::readFile(const QString &fileName)
{

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }

    reader.setDevice(&file);
    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "vCard-listing") {
                readVCardListingElement();
            } else {
                reader.raiseError(QObject::tr("Not a vCard-listing file"));
            }
        } else {
            reader.readNext();
        }
    }

    file.close();
    if (reader.hasError()) {
        std::cerr << "Error: Failed to parse file "
            << qPrintable(fileName) << ": "
            << qPrintable(reader.errorString()) << std::endl;
        return false;
    } else if (file.error() != QFile::NoError) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }
    return true;
}

void PBAPListXmlStreamReader::readVCardListingElement()
{
    reader.readNext();
    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "card") {
                readCardElement();
            }
        }
        reader.readNext();
    }
}

void PBAPListXmlStreamReader::readCardElement()
{
    QString output = reader.attributes().value("handle").toString();
    output += '\t';
    output += reader.attributes().value("name").toString();
    output += '\n';
    output += reader.readElementText();
    output += '\n';

    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(output);


    pListWidget->addItem(newItem);
}

void PBAPListXmlStreamReader::readPageElement()
{
    QString page = reader.readElementText();
    if (reader.isEndElement())
        reader.readNext();

    QString output = reader.readElementText();
    output += '\n';
}

void PBAPListXmlStreamReader::skipUnknownElement()
{
    reader.readNext();
    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            reader.readNext();
            break;
        }

        if (reader.isStartElement()) {
            skipUnknownElement();
        } else {
            reader.readNext();
        }
    }
}


//=======================================================================================


MCEFolderListXmlParser::MCEFolderListXmlParser(QListWidget *pList)
{
    pListWidget = pList;
    pComboBox = NULL;
}

MCEFolderListXmlParser::MCEFolderListXmlParser(QComboBox *pList)
{
    pComboBox = pList;
    pListWidget = NULL;
}

bool MCEFolderListXmlParser::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }

    reader.setDevice(&file);
    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "folder-listing") {
                readFolderListingElement();
            } else {
                reader.raiseError(QObject::tr("Not a folder-listing file"));
            }
        } else {
            reader.readNext();
        }
    }

    file.close();
    if (reader.hasError()) {
        std::cerr << "Error: Failed to parse file "
            << qPrintable(fileName) << ": "
            << qPrintable(reader.errorString()) << std::endl;
        return false;
    } else if (file.error() != QFile::NoError) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }
    return true;
}

void MCEFolderListXmlParser::readFolderListingElement()
{
    reader.readNext();
    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "folder") {
                readFolderElement();
            }
        }
        reader.readNext();
    }
}

void MCEFolderListXmlParser::readFolderElement()
{
    QString output = reader.attributes().value("name").toString();

    if (pListWidget)
    {
        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(output);
        newItem->setSizeHint(QSize(newItem->sizeHint().width(), 30));
        pListWidget->addItem(newItem);
    }
    else if (pComboBox)
    {
        pComboBox->addItem(output, 0);
    }
}

//====================================================================================

MCEMessageListXmlParser::MCEMessageListXmlParser(QListWidget *pList, QList<tBSA_MSG_LIST_ITEM>* pMsgList)
{
    pListWidget = pList;
    pListWidget->clear();

    pMsgListing = pMsgList;
}

bool MCEMessageListXmlParser::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }

    reader.setDevice(&file);
    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "MAP-msg-listing") {
                readMsgListingElement();
            } else {
                reader.raiseError(QObject::tr("Not a MAP-msg-listing file"));
            }
        } else {
            reader.readNext();
        }
    }

    file.close();
    if (reader.hasError()) {
        std::cerr << "Error: Failed to parse file "
            << qPrintable(fileName) << ": "
            << qPrintable(reader.errorString()) << std::endl;
        return false;
    } else if (file.error() != QFile::NoError) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }
    return true;
}

void MCEMessageListXmlParser::readMsgListingElement()
{
    reader.readNext();
    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "msg") {
                readMsgElement();
            }
        }
        reader.readNext();
    }
}

void MCEMessageListXmlParser::readMsgElement()
{
     QString handleStr = reader.attributes().value("handle").toString();

    QString output = "";

    QXmlStreamAttributes  attrs = reader.attributes();

    for(int index = 0; index < (int) attrs.size(); index++)
    {
        QXmlStreamAttribute qa = reader.attributes().at(index);

        output += qa.name().toString();
        output += '\t';
        output += qa.value().toString();
        output += '\n';
    }


    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(handleStr);
    newItem->setSizeHint(QSize(newItem->sizeHint().width(), 30));
    pListWidget->addItem(newItem);

    tBSA_MSG_LIST_ITEM mi;
    mi.handleStr = handleStr;
    mi.msginfo = output;

    pMsgListing->append(mi);

}



//====================================================================================

MCEEventReportXmlParser::MCEEventReportXmlParser(QListWidget *pList)
{
    pListWidget = pList;
    pListWidget->clear();
}

bool MCEEventReportXmlParser::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }

    reader.setDevice(&file);
    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "MAP-event-report") {
                readEventReportElement();
            } else {
                reader.raiseError(QObject::tr("Not a event report file"));
            }
        } else {
            reader.readNext();
        }
    }

    file.close();
    if (reader.hasError()) {
        std::cerr << "Error: Failed to parse file "
            << qPrintable(fileName) << ": "
            << qPrintable(reader.errorString()) << std::endl;
        return false;
    } else if (file.error() != QFile::NoError) {
        std::cerr << "Error: Cannot read file " << qPrintable(fileName)
            << ": " << qPrintable(file.errorString())
            << std::endl;
        return false;
    }
    return true;
}

void MCEEventReportXmlParser::readEventReportElement()
{
    reader.readNext();
    while (!reader.atEnd()) {
        if (reader.isStartElement()) {
            if (reader.name() == "event") {
                readEventElement();
            }
        }
        reader.readNext();
    }
}

void MCEEventReportXmlParser::readEventElement()
{
        QString output = "";

        QXmlStreamAttributes  attrs = reader.attributes();

    for(int index = 0; index < (int) attrs.size(); index++)
    {
        QXmlStreamAttribute qa = reader.attributes().at(index);

        output += qa.name().toString();
        output += '\t';
        output += qa.value().toString();
        output += '\n';
    }


    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(output);
    pListWidget->addItem(newItem);
}
