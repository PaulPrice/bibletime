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

#include "ctextrendering.h"

#include <utility>
#include "../config/btconfig.h"


namespace Rendering {

/** HTML rendering for the text display widgets.
 * @short Rendering for the html display widget.
 * @author The BibleTime team
 */

class CDisplayRendering : public CTextRendering {

public: // methods:

    static QString keyToHTMLAnchor(QString const& key);

    CDisplayRendering(
        DisplayOptions const & displayOptions = btConfig().getDisplayOptions(),
        FilterOptions const & filterOptions = btConfig().getFilterOptions());

    QString const & displayTemplateName() const noexcept
    { return m_displayTemplateName; }

    void setDisplayTemplateName(QString displayTemplateName) noexcept
    { m_displayTemplateName = std::move(displayTemplateName); }

protected: // methods:

    QString entryLink(KeyTreeItem const & item,
                      CSwordModuleInfo const & module) override;

    QString finishText(QString const & text, KeyTree const & tree) override;

private: // Fields:

    QString m_displayTemplateName;

}; /* class CDisplayRendering */

} /* namespace Rendering */
