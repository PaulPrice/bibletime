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

#include "btqmlinterface.h"

#include <QApplication>
#include <QClipboard>
#include <QScreen>
#include <QTimerEvent>
#include <utility>
#include "../../../backend/config/btconfig.h"
#include "../../../backend/drivers/cswordbookmoduleinfo.h"
#include "../../../backend/drivers/cswordlexiconmoduleinfo.h"
#include "../../../backend/drivers/cswordmoduleinfo.h"
#include "../../../backend/keys/cswordkey.h"
#include "../../../backend/managers/colormanager.h"
#include "../../../backend/managers/cdisplaytemplatemgr.h"
#include "../../../backend/managers/cswordbackend.h"
#include "../../../backend/models/btmoduletextmodel.h"
#include "../../../backend/rendering/btinforendering.h"
#include "../../../backend/rendering/cplaintextexportrendering.h"
#include "../../../backend/rendering/ctextrendering.h"
#include "../../../backend/rendering/btinforendering.h"
#include "../../../util/btconnect.h"
#include "../../bibletime.h"
#include "../../cinfodisplay.h"
#include "../../edittextwizard/btedittextwizard.h"


BtQmlInterface::BtQmlInterface(QObject* parent)
    : QObject(parent),
      m_firstHref(false),
      m_moduleTextModel(new BtModuleTextModel(this)),
      m_swordKey(nullptr),
      m_backgroundHighlightColorIndex(-1),
      m_caseSensitive(false) {

    m_moduleTextModel->setTextFilter(&m_textFilter);
}

BtQmlInterface::~BtQmlInterface() {
}

bool BtQmlInterface::isBibleOrCommentary() {
    auto const moduleType = module()->type();
    return moduleType == CSwordModuleInfo::Bible
            || moduleType == CSwordModuleInfo::Commentary;
}

void BtQmlInterface::setFilterOptions(FilterOptions filterOptions) {
    m_moduleTextModel->setFilterOptions(filterOptions);
}

int BtQmlInterface::getContextMenuIndex() const {
    return m_contextMenuIndex;
}

void BtQmlInterface::setContextMenuIndex(int index) {
    m_contextMenuIndex = index;
    Q_EMIT contextMenuIndexChanged();
}

int BtQmlInterface::getContextMenuColumn() const {
    return m_contextMenuColumn;
}

void BtQmlInterface::setContextMenuColumn(int index) {
    m_contextMenuColumn = index;
    Q_EMIT contextMenuColumnChanged();
}

QColor BtQmlInterface::getBackgroundColor() const
{ return ColorManager::getBackgroundColor(); }

QColor BtQmlInterface::getBackgroundHighlightColor() const
{ return ColorManager::getBackgroundHighlightColor(); }

int BtQmlInterface::getBackgroundHighlightColorIndex() const {
    return m_backgroundHighlightColorIndex;
}

QColor BtQmlInterface::getForegroundColor() const
{ return ColorManager::getForegroundColor(); }

int BtQmlInterface::getCurrentModelIndex() const {
    if (m_swordKey == nullptr)
        return 0;
    auto const moduleType = module()->type();
    if (moduleType == CSwordModuleInfo::Bible
        || moduleType == CSwordModuleInfo::Commentary)
    {
        CSwordVerseKey* verseKey = dynamic_cast<CSwordVerseKey*>(m_swordKey);
        int index = m_moduleTextModel->verseKeyToIndex(*verseKey);
        return index;
    } else if (moduleType == CSwordModuleInfo::GenericBook) {
        const CSwordBookModuleInfo *m = qobject_cast<const CSwordBookModuleInfo*>(module());
        CSwordTreeKey key(m->tree(), m);
        QString keyName = m_swordKey->key();
        key.setKey(keyName);
        CSwordTreeKey p(key);
        p.positionToRoot();
        if(p != key)
            return static_cast<int>(key.offset() / 4u); /// \todo Check range!
    } else if (moduleType == CSwordModuleInfo::Lexicon) {
        const CSwordLexiconModuleInfo *li =
                qobject_cast<const CSwordLexiconModuleInfo*>(m_swordKey->module());
        int index = li->entries().indexOf(m_swordKey->key());
        return index;
    }
    return 0;
}

int BtQmlInterface::getNumModules() const {
    return m_moduleNames.count();
}

double BtQmlInterface::getPixelsPerMM() const {
    QScreen* screen = QGuiApplication::screens().at(0);
    double millimeterPerInch = 25.4;
    return screen->physicalDotsPerInchX() / millimeterPerInch;
}

QString BtQmlInterface::stripHtml(const QString& html) {
    QString t = html;
    //since t is a complete HTML page at the moment, strip away headers and footers of a HTML page
    QRegExp re("(?:<html.*>.+<body.*>)", Qt::CaseInsensitive); //remove headers, case insensitive
    re.setMinimal(true);
    t.replace(re, "");
    t.replace(QRegExp("</body></html>", Qt::CaseInsensitive), "");//remove footer
    return t;
}

QString BtQmlInterface::getRawText(int row, int column) {
    BT_ASSERT(column >= 0 && column <= m_moduleNames.count());
    CSwordVerseKey key = m_moduleTextModel->indexToVerseKey(row);
    QString moduleName = m_moduleNames.at(column);
    CSwordModuleInfo* module = CSwordBackend::instance()->findModuleByName(moduleName);
    CSwordVerseKey mKey(module);
    mKey.setKey(key.key());
    QString rawText = mKey.rawText();
    return stripHtml(rawText);
}

void BtQmlInterface::openEditor(int row, int column) {
    BtEditTextWizard wiz;
    wiz.setTitle(tr("Edit %1").arg(m_moduleTextModel->indexToKeyName(row)));
    wiz.setText(getRawText(row, column));
    wiz.setFont(m_fonts.at(column));
    if (wiz.exec() == QDialog::Accepted)
        setRawText(row, column, wiz.text());
}

int BtQmlInterface::indexToVerse(int index) {
    return m_moduleTextModel->indexToVerse(index);
}

void BtQmlInterface::setHoveredLink(QString const & link) {
    if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
        return;
    setMagReferenceByUrl(link);
    m_activeLink = link;
}

QString BtQmlInterface::getLemmaFromLink(const QString& url) {
    QString reference;

    QRegExp rx("sword://lemmamorph/([a-s]+)=([GH][0-9]+)");
    rx.setMinimal(false);
    int pos1 = rx.indexIn(url);
    if (pos1 > -1) {
        reference = rx.cap(2);
    }
    return reference;
}

QString BtQmlInterface::getBibleUrlFromLink(const QString& url) {
    QString reference;

    QRegExp rx("(sword://Bible/.*)\\|\\|(.*)=(.*)");
    rx.setMinimal(false);
    int pos1 = rx.indexIn(url);
    if (pos1 > -1) {
        reference = rx.cap(1);
    }
    return reference;
}

QString BtQmlInterface::getReferenceFromUrl(const QString& url) {
    {
        QRegExp rx("sword://(bible|lexicon)/(.*)/(.*)(\\|\\|)",
                   Qt::CaseInsensitive);
        rx.setMinimal(false);
        if (rx.indexIn(url) > -1)
            return "href=sword://" + rx.cap(1) + "/" + rx.cap(2) + "/"
                   + rx.cap(3);
    }{
        QRegExp rx("sword://(bible|lexicon)/(.*)/(.*)", Qt::CaseInsensitive);
        rx.setMinimal(false);
        if (rx.indexIn(url) > -1)
            return "href=sword://" + rx.cap(1) + "/" + rx.cap(2) + "/"
                   + rx.cap(3);
    }{
        QRegExp rx("sword://footnote/(.*)=(.*)", Qt::CaseInsensitive);
        rx.setMinimal(false);
        if (rx.indexIn(url) > -1)
            return "note=" + rx.cap(1);
    }{
        QRegExp rx("sword://lemmamorph/(.*)=(.*)/(.*)", Qt::CaseInsensitive);
        rx.setMinimal(false);
        if (rx.indexIn(url) > -1)
            return rx.cap(1) + "=" + rx.cap(2);
    }
    return {};
}

void BtQmlInterface::setRawText(int row, int column, const QString& text) {
    QModelIndex index = m_moduleTextModel->index(row, 0);
    int role = ModuleEntry::Text0Role + column;
    m_moduleTextModel->setData(index, text, role);
}

void BtQmlInterface::cancelMagTimer() {
    if (m_linkTimerId) {
        killTimer(*m_linkTimerId);
        m_linkTimerId.reset();
    }
}

void BtQmlInterface::setMagReferenceByUrl(const QString& url) {
    if (url.isEmpty())
        return;
    m_timeoutUrl = url;
    cancelMagTimer();
    if (auto const timerId = startTimer(400))
        m_linkTimerId.emplace(timerId);
}

void BtQmlInterface::settingsChanged() {
    getFontsFromSettings();
    changeColorTheme();
    Q_EMIT textChanged();
}

void BtQmlInterface::pageDown() {
    Q_EMIT pageDownChanged();
}

void BtQmlInterface::pageUp() {
    Q_EMIT pageUpChanged();
}

void BtQmlInterface::getFontsFromSettings() {
    m_fonts.clear();
    for(int i=0; i<m_moduleNames.count(); ++i) {
        QString moduleName = m_moduleNames.at(i);
        QFont font;
        CSwordModuleInfo* m = CSwordBackend::instance()->findModuleByName(moduleName);
        if (m != nullptr) {
            if (auto const lang = m->language()) {
                BtConfig::FontSettingsPair fontPair = btConfig().getFontForLanguage(*lang);
                if (fontPair.first) {
                    font = fontPair.second;
                } else {
                    font = btConfig().getDefaultFont();
                }
            }
        }
        m_fonts.append(font);
        Q_EMIT fontChanged();
    }
}

/** Sets the new sword key. */
void BtQmlInterface::setKey( CSwordKey* key ) {
    BT_ASSERT(key);
    m_swordKey = key;
}

void BtQmlInterface::setKeyFromLink(const QString& link) {
    QRegExp rx("sword://Bible/(.*)/(.*)\\|\\|(.*)=(.*)");
    rx.setMinimal(false);
    int pos1 = rx.indexIn(link);
    QString keyName;
    if (pos1 > -1) {
        keyName = rx.cap(2);
        Q_EMIT newBibleReference(keyName);
    }
}

void BtQmlInterface::scrollToSwordKey(CSwordKey * key) {
    m_backgroundHighlightColorIndex = m_moduleTextModel->keyToIndex(*key);

    /* Convert from sword index to ListView index */
    m_backgroundHighlightColorIndex = m_backgroundHighlightColorIndex - m_moduleTextModel->getFirstEntryIndex();

    Q_EMIT backgroundHighlightColorIndexChanged();
    m_swordKey = key;
    Q_EMIT currentModelIndexChanged();
}

void BtQmlInterface::setModules(const QStringList &modules) {
    m_moduleNames = modules;
    m_moduleTextModel->setModules(modules);
    getFontsFromSettings();
    Q_EMIT numModulesChanged();
}

void BtQmlInterface::referenceChosen() {
    Q_EMIT currentModelIndexChanged();
}

void BtQmlInterface::changeReference(int i) {
    QString reference = m_moduleTextModel->indexToKeyName(i);
    Q_EMIT updateReference(reference);
}

void BtQmlInterface::dragHandler(int index) {
    QString moduleName;
    QString keyName;

    QRegExp rx("sword://Bible/(.*)/(.*)\\|\\|(.*)=(.*)");
    rx.setMinimal(false);
    int pos1 = rx.indexIn(m_activeLink);

    if (pos1 > -1) {
        moduleName = rx.cap(1);
        keyName = rx.cap(2);
    } else {
        moduleName = m_moduleNames.at(0);
        keyName = m_moduleTextModel->indexToKeyName(index);
    }

    Q_EMIT dragOccuring(moduleName, keyName);
}

const CSwordModuleInfo* BtQmlInterface::module() const
{ return m_swordKey ? m_swordKey->module() : nullptr; }

CSwordKey* BtQmlInterface::getKey() const {
    return m_swordKey;
}

CSwordKey* BtQmlInterface::getMouseClickedKey() const {
    return m_moduleTextModel->indexToKey(m_contextMenuIndex, m_contextMenuColumn);
}

QFont BtQmlInterface::font(int column) const {
    if (column >= 0 && column < m_fonts.count())
        return m_fonts.at(column);
    return QApplication::font();
}

QFont BtQmlInterface::getFont0() const { return font(0); }
QFont BtQmlInterface::getFont1() const { return font(1); }
QFont BtQmlInterface::getFont2() const { return font(2); }
QFont BtQmlInterface::getFont3() const { return font(3); }
QFont BtQmlInterface::getFont4() const { return font(4); }
QFont BtQmlInterface::getFont5() const { return font(5); }
QFont BtQmlInterface::getFont6() const { return font(6); }
QFont BtQmlInterface::getFont7() const { return font(7); }
QFont BtQmlInterface::getFont8() const { return font(8); }
QFont BtQmlInterface::getFont9() const { return font(9); }

QString BtQmlInterface::getSelectedText() {

    QString text;
    for (auto const & value : m_selectedText)
        text.append(value).append('\n');
    return text;
}

QVariant BtQmlInterface::getTextModel() {
    QVariant var;
    var.setValue(m_moduleTextModel);
    return var;
}

BtModuleTextModel * BtQmlInterface::textModel() {
    return m_moduleTextModel;
}

BtModuleTextModel const * BtQmlInterface::textModel() const
{ return m_moduleTextModel; }

bool BtQmlInterface::moduleIsWritable(int column) {
    if (column >= m_moduleNames.count())
        return false;
    QString moduleName = m_moduleNames.at(column);
    CSwordModuleInfo* module = CSwordBackend::instance()->findModuleByName(moduleName);
    return module->isWritable();
}

void BtQmlInterface::changeColorTheme() {
    Q_EMIT backgroundHighlightColorChanged();
    Q_EMIT backgroundColorChanged();
    Q_EMIT foregroundColorChanged();
}

void BtQmlInterface::copyRange(int index1, int index2) const {
    QString text;
    std::unique_ptr<CSwordKey> key(m_swordKey->copy());

    for (int i=index1; i<=index2; ++i) {
        QString keyName = m_moduleTextModel->indexToKeyName(i);
        key->setKey(keyName);
        text += keyName + "\n" + key->strippedText() + "\n\n";
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

void BtQmlInterface::copyVerseRange(QString const & ref1,
                                    QString const & ref2,
                                    CSwordModuleInfo const & module) const
{
    CSwordVerseKey dummy(&module);
    CSwordVerseKey vk(&module);
    dummy.setKey(ref1);
    vk.setLowerBound(dummy);
    dummy.setKey(ref2);
    vk.setUpperBound(dummy);

    // Copy key:
    if (!vk.module())
        return;

    auto const render =
            [](){
                DisplayOptions displayOptions;
                displayOptions.lineBreaks = true;
                displayOptions.verseNumbers = true;

                FilterOptions filterOptions;
                filterOptions.footnotes = 0;
                filterOptions.greekAccents = 1;
                filterOptions.headings = 1;
                filterOptions.hebrewCantillation = 1;
                filterOptions.hebrewPoints = 1;
                filterOptions.lemmas = 0;
                filterOptions.morphSegmentation = 1;
                filterOptions.morphTags = 0;
                filterOptions.redLetterWords = 1;
                filterOptions.scriptureReferences = 0;
                filterOptions.strongNumbers = 0;
                filterOptions.textualVariants = 0;

                return std::make_unique<Rendering::CPlainTextExportRendering>(
                            true,
                            displayOptions,
                            filterOptions);
            }();
    QGuiApplication::clipboard()->setText(
                vk.isBoundSet()
                ? render->renderKeyRange(vk.lowerBound(),
                                         vk.upperBound(),
                                         {&module})
                : render->renderSingleKey(vk.key(), {&module}));
}

QString BtQmlInterface::getHighlightWords() const {
    return m_highlightWords;
}

void BtQmlInterface::setHighlightWords(const QString& words, bool caseSensitive) {
    m_highlightWords = words;
    m_caseSensitive = caseSensitive;
    QTimer::singleShot(900, this, &BtQmlInterface::slotSetHighlightWords);
}

void BtQmlInterface::slotSetHighlightWords() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_moduleTextModel->setHighlightWords(m_highlightWords, m_caseSensitive);
    m_findState.reset();
    m_moduleTextModel->setFindState(m_findState);
    Q_EMIT highlightWordsChanged();
    QApplication::restoreOverrideCursor();
}

void BtQmlInterface::timerEvent(QTimerEvent * const event) {
    if (m_linkTimerId && event->timerId() == *m_linkTimerId) {
        event->accept();
        cancelMagTimer();
        auto infoList(Rendering::detectInfo(getReferenceFromUrl(m_timeoutUrl)));
        if (!infoList.isEmpty())
            BibleTime::instance()->infoDisplay()->setInfo(std::move(infoList));
    } else {
        QObject::timerEvent(event);
    }
}

void BtQmlInterface::findText(const QString& /*text*/,
                              bool /*caseSensitive*/, bool backward) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (!m_findState)
        m_findState = FindState{getCurrentModelIndex(), 0};

    if (backward)
        getPreviousMatchingItem(m_findState->index);
    else
        getNextMatchingItem(m_findState->index);

    m_moduleTextModel->setFindState(m_findState);
    Q_EMIT highlightWordsChanged();
    Q_EMIT positionItemOnScreen(m_findState->index);
    QApplication::restoreOverrideCursor();
}

int BtQmlInterface::countHighlightsInItem(int index) {
    QModelIndex mIndex = m_moduleTextModel->index(index);
    QString text = m_moduleTextModel->data(mIndex, ModuleEntry::Text1Role).toString();
    int num = text.count("\"highlightwords");
    return num;
}

void BtQmlInterface::getNextMatchingItem(int startIndex) {
    int num = countHighlightsInItem(startIndex);
    if (num > m_findState->subIndex) { // Found within startIndex item
        m_findState->index = startIndex;
        ++m_findState->subIndex;
        return;
    }

    if (startIndex >= m_moduleTextModel->rowCount())
        return;

    int index = startIndex+1;
    for (int i = 0; i < 1000; ++i) {
        int num = countHighlightsInItem(index);
        if (num > 0 ) {
            m_findState->index = index;
            m_findState->subIndex = 1;
            return;
        }
        ++index;
    }
    return;
}

void BtQmlInterface::getPreviousMatchingItem(int startIndex) {
    int num = countHighlightsInItem(startIndex);
    if (num > 0 && m_findState->subIndex == 0) {
        // Found within startIndex item
        m_findState->index = startIndex;
        m_findState->subIndex = 1;
        return;
    }

    if (startIndex <= 0)
        return;

    int index = startIndex;
    if (m_findState->subIndex == 0)
        --index;
    for (int i = 0; i < 1000; ++i) {
        int num = countHighlightsInItem(index);
        if (num > 0 ) {
            m_findState->index = index;
            if (m_findState->subIndex == 0)
                m_findState->subIndex = num;
            else
                --m_findState->subIndex;
            if (m_findState->subIndex != 0)
                return;
        }
        --index;
    }
    return;
}

bool BtQmlInterface::hasSelectedText() {
    return ! m_selectedText.isEmpty();
}

void BtQmlInterface::clearSelectedText() {
    m_selectedText.clear();
}

void BtQmlInterface::saveSelectedText(int index, const QString& text) {
    m_selectedText.insert(index, text);
}
