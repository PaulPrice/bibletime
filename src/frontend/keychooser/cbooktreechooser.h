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

#include "ckeychooser.h"

#include <QList>
#include <QObject>
#include <QString>
#include "../../backend/drivers/btmodulelist.h"
#include "../../backend/keys/cswordtreekey.h"


class BTHistory;
class CSwordBookModuleInfo;
class CSwordKey;
class QTreeWidget;
class QTreeWidgetItem;
class QWidget;

class CBookTreeChooser final : public CKeyChooser {

    Q_OBJECT

public: // methods:

    CBookTreeChooser(BtConstModuleList const & modules,
                     BTHistory * history,
                     CSwordKey * key = nullptr,
                     QWidget * parent = nullptr);

    void refreshContent() final override;
    void setModules(BtConstModuleList const & modules,
                    bool refresh = true) final override;

    CSwordKey * key() final override { return m_key; }
    void setKey(CSwordKey * key) final override;
    void setKey(CSwordKey *, const bool emitSinal);

public Q_SLOTS:

    void updateKey(CSwordKey *) final override;
    void doShow();

private: // methods:

    /** \brief Creates the first level of the tree structure. */
    void setupTree();
    void addKeyChildren(CSwordTreeKey * key, QTreeWidgetItem * item);
    void adjustFont();
    void handleHistoryMoved(QString const & newKey) final override;

private Q_SLOTS:

    void itemActivated(QTreeWidgetItem * item);

private: // fields:

    QList<CSwordBookModuleInfo const *> m_modules;
    CSwordTreeKey * m_key;
    QTreeWidget * m_treeView;

}; /* class CBookTreeChooser */
