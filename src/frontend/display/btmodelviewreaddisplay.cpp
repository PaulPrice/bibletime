/*********
*
* In the name of the Father, and of the Son, and of the Holy Spirit.
*
* This file is part of BibleTime's source code, https://bibletime.info/
*
* Copyright 1999-2021 by the BibleTime developers.
* The BibleTime source code is licensed under the GNU General Public License
* version 2.0.
*
**********/

#include "btmodelviewreaddisplay.h"

#include <memory>
#include <QClipboard>
#include <QDebug>
#include <QDrag>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QString>
#include <QTimer>
#include <QToolBar>
#include "../../backend/keys/cswordkey.h"
#include "../../backend/drivers/cswordbiblemoduleinfo.h"
#include "../../backend/managers/referencemanager.h"
#include "../../util/btassert.h"
#include "../../util/btconnect.h"
#include "../../util/directory.h"
#include "../../util/tool.h"
#include "../btcopybyreferencesdialog.h"
#include "../bibletime.h"
#include "../BtMimeData.h"
#include "../cexportmanager.h"
#include "../cinfodisplay.h"
#include "../cmdiarea.h"
#include "../displaywindow/cdisplaywindow.h"
#include "../keychooser/ckeychooser.h"
#include "modelview/btqmlscrollview.h"
#include "modelview/btqmlinterface.h"
#include "modelview/btquickwidget.h"


using namespace InfoDisplay;

BtModelViewReadDisplay::BtModelViewReadDisplay(CDisplayWindow * displayWindow,
                                               QWidget * parentWidget)
    : QWidget(parentWidget)
    , m_parentWindow(displayWindow)
    , m_popup(nullptr)
    , m_widget(nullptr)
{
    setObjectName("BtModelViewReadDisplay");
    QHBoxLayout* layout = new QHBoxLayout(this);
    setLayout(layout);
    m_widget = new BtQmlScrollView(this, this);
    layout->addWidget(m_widget);
    m_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    BT_CONNECT(m_widget->qmlInterface(), &BtQmlInterface::updateReference,
               [this](QString const & reference) {
                   auto * const key = m_parentWindow->key();
                   key->setKey(reference);
                   m_parentWindow->keyChooser()->updateKey(key);
                   m_parentWindow->setWindowTitle(
                               m_parentWindow->windowCaption());
               });
    BT_CONNECT(m_widget->qmlInterface(), &BtQmlInterface::dragOccuring,
               [this](const QString& moduleName, const QString& keyName) {
                   auto & drag = *new QDrag(this);
                   auto mimedata =
                           std::make_unique<BTMimeData>(
                               BTMimeData::ItemList{{moduleName, keyName, {}}});
                   //add real Bible text from module/key
                   if (auto * const module =
                        CSwordBackend::instance()->findModuleByName(moduleName))
                   {
                       drag.setPixmap(
                               module->moduleIcon().pixmap(
                                   m_parentWindow->mainToolBar()->iconSize()));
                       std::unique_ptr<CSwordKey> key(module->createKey());
                       key->setKey(keyName);
                       // This works across applications:
                       mimedata->setText(key->strippedText());
                   }
                   drag.setMimeData(mimedata.release());
                   drag.exec(Qt::CopyAction, Qt::CopyAction);
               });
    BT_CONNECT(m_widget, &BtQmlScrollView::referenceDropped,
               [this](QString const & reference) { /// \todo Fix me
                   auto key(m_parentWindow->key());
                   key->setKey(reference);
                   m_parentWindow->lookupKey(reference);
               });
}

BtModelViewReadDisplay::~BtModelViewReadDisplay() = default;

void BtModelViewReadDisplay::copyAsPlainText(TextPart const part)
{ QGuiApplication::clipboard()->setText(text(part)); }

void BtModelViewReadDisplay::copySelectedText()
{ QGuiApplication::clipboard()->setText(qmlInterface()->getSelectedText()); }

void BtModelViewReadDisplay::copyByReferences() {
    auto const & qml = *qmlInterface();
    BtCopyByReferencesDialog dlg(qml.textModel(), m_parentWindow);
    if (dlg.exec() != QDialog::Accepted)
        return;

    auto const & result = dlg.result();
    BT_ASSERT(result.module);
    auto const & module = *result.module;
    if (module.type() == CSwordModuleInfo::Bible
        || module.type() == CSwordModuleInfo::Commentary)
    {
        qml.copyVerseRange(result.reference1, result.reference2, module);
    } else {
        qml.copyRange(result.index1, result.index2);
    }
}

void BtModelViewReadDisplay::save(TextPart const part) {
    auto const filename =
            QFileDialog::getSaveFileName(
                nullptr,
                QObject::tr("Save document ..."),
                "",
                QObject::tr("Text files") + " (*.txt);;"
                + QObject::tr("All files") + " (*)");
    if (!filename.isEmpty())
        util::tool::savePlainFile(filename, text(part));
}

void BtModelViewReadDisplay::print(TextPart const type,
                                   DisplayOptions const & displayOptions,
                                   FilterOptions const & filterOptions)
{
    using CSBiMI = CSwordBibleModuleInfo;
    CSwordKey* const key = m_parentWindow->key();
    const CSwordModuleInfo *module = key->module();

    CExportManager mgr(false,
                       QString(),
                       m_parentWindow->filterOptions(),
                       m_parentWindow->displayOptions());

    switch (type) {
    case Document: {
        if (module->type() == CSwordModuleInfo::Bible) {
            CSwordVerseKey* vk = dynamic_cast<CSwordVerseKey*>(key);

            CSwordVerseKey startKey(*vk);
            startKey.setVerse(1);

            CSwordVerseKey stopKey(*vk);

            const CSBiMI *bible = dynamic_cast<const CSBiMI*>(module);
            if (bible) {
                stopKey.setVerse(bible->verseCount(bible->bookNumber(startKey.bookName()), startKey.chapter()));
            }

            mgr.printKey(module, startKey.key(), stopKey.key(), displayOptions, filterOptions);
        }
        else if (module->type() == CSwordModuleInfo::Lexicon || module->type() == CSwordModuleInfo::Commentary ) {
            mgr.printKey(module, key->key(), key->key(), displayOptions, filterOptions);
        }
        else if (module->type() == CSwordModuleInfo::GenericBook) {
            CSwordTreeKey* tree = dynamic_cast<CSwordTreeKey*>(key);

            CSwordTreeKey startKey(*tree);
            //        while (startKey.previousSibling()) { // go to first sibling on this level!
            //        }

            CSwordTreeKey stopKey(*tree);
            //    if (CSwordBookModuleInfo* book = dynamic_cast<CSwordBookModuleInfo*>(module)) {
            //          while ( stopKey.nextSibling() ) { //go to last displayed sibling!
            //          }
            //        }
            mgr.printKey(module, startKey.key(), stopKey.key(), displayOptions, filterOptions);
        }
        break;
    }

    case AnchorWithText: {
        if (hasActiveAnchor()) {
            mgr.printByHyperlink(m_activeAnchor, displayOptions, filterOptions );
        }
        break;
    }

    default:
        break;
    }
}

void BtModelViewReadDisplay::reloadModules() {
    qmlInterface()->textModel()->reloadModules();
}

QString
BtModelViewReadDisplay::text(TextPart const part) {
    QString text;
    switch (part) {
    case Document: {
        CSwordKey* const key = m_parentWindow->key();
        const CSwordModuleInfo *module = key->module();
        //This is never used for Bibles, so it is not implemented for
        //them.  If it should be, see CReadDisplay::print() for example
        //code.
        BT_ASSERT(module->type() == CSwordModuleInfo::Lexicon ||
                  module->type() == CSwordModuleInfo::Commentary ||
                  module->type() == CSwordModuleInfo::GenericBook);
        FilterOptions filterOptions;
        CSwordBackend::instance()->setFilterOptions(filterOptions);

        text = QString(key->strippedText()).append("\n(")
                .append(key->key())
                .append(", ")
                .append(key->module()->name())
                .append(")");
        break;
    }

    case AnchorOnly: {
        if (auto const decodedLink =
                    ReferenceManager::decodeHyperlink(m_activeAnchor))
            return decodedLink->key;
        return {};
    }

    case AnchorTextOnly: {
        auto const decodedLink(
                    ReferenceManager::decodeHyperlink(m_activeAnchor));
        if (decodedLink && decodedLink->module) {
            std::unique_ptr<CSwordKey> key(decodedLink->module->createKey());
            key->setKey(decodedLink->key);
            return key->strippedText();
        }
        return {};
    }

    case AnchorWithText: {
        auto const decodedLink(
                    ReferenceManager::decodeHyperlink(m_activeAnchor));
        if (decodedLink && decodedLink->module) {
            std::unique_ptr<CSwordKey> key(decodedLink->module->createKey());
            key->setKey(decodedLink->key);

            FilterOptions filterOptions;
            CSwordBackend::instance()->setFilterOptions(filterOptions);

            return QString(key->strippedText()).append("\n(")
                    .append(key->key())
                    .append(", ")
                    .append(key->module()->name())
                    .append(")");
        }
        return {};
    }
    default:
        break;
    }
    return QString();

}

void BtModelViewReadDisplay::setDisplayFocus() {
    m_widget->quickWidget()->setFocus();
}

void BtModelViewReadDisplay::setDisplayOptions(const DisplayOptions &displayOptions) {
    m_widget->qmlInterface()->textModel()->setDisplayOptions(displayOptions);
}

void BtModelViewReadDisplay::contextMenu(QContextMenuEvent* event) {
    auto const & activeLink = m_widget->qmlInterface()->activeLink();
    QString reference = m_widget->qmlInterface()->getBibleUrlFromLink(activeLink);
    m_activeAnchor = reference;
    QString lemma = m_widget->qmlInterface()->getLemmaFromLink(activeLink);
    setLemma(lemma);

    if (m_popup)
        m_popup->exec(event->globalPos());
}

BtQmlInterface * BtModelViewReadDisplay::qmlInterface() const {
    return m_widget->qmlInterface();
}

void BtModelViewReadDisplay::setModules(const QStringList &modules) {
    m_widget->qmlInterface()->setModules(modules);
}

void BtModelViewReadDisplay::scrollToKey(CSwordKey * key) {
    m_widget->scrollToSwordKey(key);
}

void BtModelViewReadDisplay::scroll(int value) {
    m_widget->quickWidget()->scroll(value);
}

void BtModelViewReadDisplay::setFilterOptions(FilterOptions filterOptions) {
    m_widget->setFilterOptions(filterOptions);
}

void BtModelViewReadDisplay::settingsChanged() {
    m_widget->settingsChanged();
}

void BtModelViewReadDisplay::updateReferenceText() {
    m_widget->quickWidget()->updateReferenceText();
}

void BtModelViewReadDisplay::pageDown() {
    m_widget->pageDown();
}

void BtModelViewReadDisplay::pageUp() {
    m_widget->pageUp();
}

void BtModelViewReadDisplay::highlightText(const QString& text, bool caseSensitive) {
    m_widget->qmlInterface()->setHighlightWords(text, caseSensitive);
}

void BtModelViewReadDisplay::findText(const QString& text,
                                      bool caseSensitive, bool backward) {
    m_widget->qmlInterface()->findText(text, caseSensitive, backward);
}

// Save the Lemma (Strongs number) attribute
void BtModelViewReadDisplay::setLemma(const QString& lemma) {
    m_nodeInfo = lemma;
}

// Open the Find text dialog
void BtModelViewReadDisplay::openFindTextDialog()
{ m_parentWindow->btMainWindow()->openFindWidget(); }
