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

#pragma once

#include <memory>
#include <optional>
#include <QFont>
#include <QList>
#include <QObject>
#include <QMap>
#include <QString>
#include "../../../backend/rendering/ctextrendering.h"
#include "bttextfilter.h"


class CSwordKey;
class CSwordModuleInfo;

class RoleItemModel;

/**
 * /brief This class provides communications between QML and c++.
 *
 * It is instantiated by usage within the QML files. It provides
 * properties and functions written in c++ and usable by QML.
 */

class BtQmlInterface : public QObject {

    Q_OBJECT
    Q_PROPERTY(QColor       backgroundColor         READ getBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor       backgroundHighlightColor READ getBackgroundHighlightColor NOTIFY backgroundHighlightColorChanged)
    Q_PROPERTY(int          backgroundHighlightColorIndex READ getBackgroundHighlightColorIndex NOTIFY backgroundHighlightColorIndexChanged)
    Q_PROPERTY(int          contextMenuIndex        READ getContextMenuIndex NOTIFY contextMenuIndexChanged WRITE setContextMenuIndex)
    Q_PROPERTY(int          contextMenuColumn       READ getContextMenuColumn NOTIFY contextMenuColumnChanged WRITE setContextMenuColumn)
    Q_PROPERTY(int          currentModelIndex       READ getCurrentModelIndex NOTIFY currentModelIndexChanged)
    Q_PROPERTY(QFont        font0                   READ getFont0   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font1                   READ getFont1   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font2                   READ getFont2   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font3                   READ getFont3   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font4                   READ getFont4   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font5                   READ getFont5   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font6                   READ getFont6   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font7                   READ getFont7   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font8                   READ getFont8   NOTIFY fontChanged)
    Q_PROPERTY(QFont        font9                   READ getFont9   NOTIFY fontChanged)
    Q_PROPERTY(QColor       foregroundColor         READ getForegroundColor NOTIFY foregroundColorChanged)
    Q_PROPERTY(QString      highlightWords          READ getHighlightWords NOTIFY highlightWordsChanged)
    Q_PROPERTY(int          numModules              READ getNumModules NOTIFY numModulesChanged)
    Q_PROPERTY(double       pixelsPerMM             READ getPixelsPerMM NOTIFY pixelsPerMMChanged)
    Q_PROPERTY(QVariant     textModel               READ getTextModel NOTIFY textModelChanged)

public:
    void cancelMagTimer();
    Q_INVOKABLE void changeReference(int i);
    Q_INVOKABLE void dragHandler(int index);
    QString getRawText(int row, int column);
    Q_INVOKABLE bool moduleIsWritable(int column);
    Q_INVOKABLE void openEditor(int row, int column);
    void referenceChosen();
    void setMagReferenceByUrl(const QString& url);
    Q_INVOKABLE void setRawText(int row, int column, const QString& text);
    Q_INVOKABLE void clearSelectedText();
    Q_INVOKABLE bool hasSelectedText();
    Q_INVOKABLE void saveSelectedText(int index, const QString& text);
    Q_INVOKABLE void setKeyFromLink(const QString& link);
    Q_INVOKABLE int indexToVerse(int index);
    Q_INVOKABLE void setHoveredLink(QString const & link);

    BtQmlInterface(QObject *parent = nullptr);
    ~BtQmlInterface() override;

    int countHighlightsInItem(int index);
    void findText(const QString& text, bool caseSensitive, bool backward);
    void getNextMatchingItem(int index);
    void getPreviousMatchingItem(int index);


    QString const & activeLink() const noexcept { return m_activeLink; }
    QColor getBackgroundColor() const;
    QColor getBackgroundHighlightColor() const;
    QColor getForegroundColor() const;
    int getBackgroundHighlightColorIndex() const;
    void changeColorTheme();
    void copyRange(int index1, int index2) const;
    void copyVerseRange(QString const & ref1,
                        QString const & ref2,
                        CSwordModuleInfo const & module) const;
    QString getBibleUrlFromLink(const QString& url);
    int getContextMenuIndex() const;
    int getContextMenuColumn() const;
    int getCurrentModelIndex() const;
    QFont getFont0() const;
    QFont getFont1() const;
    QFont getFont2() const;
    QFont getFont3() const;
    QFont getFont4() const;
    QFont getFont5() const;
    QFont getFont6() const;
    QFont getFont7() const;
    QFont getFont8() const;
    QFont getFont9() const;
    QString getHighlightWords() const;
    CSwordKey* getKey() const;
    CSwordKey* getMouseClickedKey() const;
    QString getLemmaFromLink(const QString& url);
    int getNumModules() const;
    double getPixelsPerMM() const;
    QString getSelectedText();
    QVariant getTextModel();
    bool isBibleOrCommentary();
    BtModuleTextModel * textModel();
    BtModuleTextModel const * textModel() const;
    void pageDown();
    void pageUp();
    void referenceChoosen();
    void scrollToSwordKey(CSwordKey * key);
    void setContextMenuIndex(int index);
    void setContextMenuColumn(int index);
    void setFilterOptions(FilterOptions filterOptions);
    void setHighlightWords(const QString& words, bool caseSensitivy);
    void setKey(CSwordKey* key);
    void setModules(const QStringList &modules);
    void settingsChanged();

Q_SIGNALS:
    void backgroundColorChanged();
    void backgroundHighlightColorChanged();
    void backgroundHighlightColorIndexChanged();
    void contextMenuIndexChanged();
    void contextMenuColumnChanged();
    void currentModelIndexChanged();
    void fontChanged();
    void foregroundColorChanged();
    void highlightWordsChanged();
    void numModulesChanged();
    void pageDownChanged();
    void pageUpChanged();
    void pixelsPerMMChanged();
    void positionItemOnScreen(int index);
    void newBibleReference(const QString& reference);
    void textChanged();
    void textModelChanged();
    void updateReference(const QString& reference);
    void dragOccuring(const QString& moduleName, const QString& keyName);

protected: // Methods:

    void timerEvent(QTimerEvent * event) final override;

private Q_SLOTS:
    void slotSetHighlightWords();

private:
    QString decodeLemma(const QString& value);
    QString decodeMorph(const QString& value);
    QFont font(int column) const;
    void getFontsFromSettings();
    QString getReferenceFromUrl(const QString& url);
    const CSwordModuleInfo* module() const;
    QString stripHtml(const QString& html);

private: // Fields:

    bool m_firstHref;
    std::optional<int> m_linkTimerId;
    BtModuleTextModel* m_moduleTextModel;
    CSwordKey* m_swordKey;

    QList<QFont> m_fonts;
    int m_backgroundHighlightColorIndex;
    bool m_caseSensitive;
    QString m_highlightWords;
    QStringList m_moduleNames;
    BtTextFilter m_textFilter;
    QString m_timeoutUrl;
    int m_contextMenuIndex;
    int m_contextMenuColumn;
    QString m_activeLink;
    std::optional<FindState> m_findState;
    QMap<int, QString> m_selectedText;
};
