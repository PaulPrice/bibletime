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
#include <QObject>
#include <QString>
#include <QStringList>
#include <set>
#include "../../util/btassert.h"
#include "../bookshelfmodel/btbookshelfmodel.h"
#include "../drivers/cswordmoduleinfo.h"
#include "../drivers/btconstmoduleset.h"
#include "../filters/gbftohtml.h"
#include "../filters/osistohtml.h"
#include "../filters/plaintohtml.h"
#include "../filters/teitohtml.h"
#include "../filters/thmltohtml.h"
#include "../language.h"
#include "../rendering/cbookdisplay.h"
#include "../rendering/cchapterdisplay.h"
#include "../rendering/centrydisplay.h"

// Sword includes:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#include <swmgr.h>
#pragma GCC diagnostic pop

namespace sword {
class Module;
class Config;
} // namespace sword

/**
  \brief The backend layer main class, a backend implementation of Sword.

  This is the implementation of CBackend for Sword. It's additionally derived
  from SWMgr to provide functions of Sword.

  \note Mostly, only one instance of this class is used. This instance is
        created by BibleTime::initBackends() and is destroyed by
        BibleTimeApp::~BibleTimeApp(). Only when \ref BackendNotSingleton
        "managing modules" separate backends are created.
*/
class CSwordBackend: public QObject {

    Q_OBJECT

public: // types:

    /**
      \brief The reason for the sigSwordSetupChanged signal, i.e. why the module
             list has changed.
    */
    enum SetupChangedReason {
        AddedModules = 1,
        RemovedModules = 2,
        HidedModules = 4,
        PathChanged = 8,
        OtherChange = 16
    };

    /**
      \brief The error codes which may be returned by the \ref Load() call.
      \note These values exist to cast from the char return of SWMgr::Load().
    */
    enum LoadError {
        NoSwordConfig = -1,
        NoError = 0,
        NoModules = 1
    };

private: // types:

    using AvailableLanguagesCacheContainer =
            std::set<std::shared_ptr<Language const>>;

public: // methods:

    /**
      \note Used by BtInstallBackend only.
      \note Using augmentHome=false can mess up the system because it is true
            elsewhere.
      \param[in] path The path which is used to load modules.
      \param[in] augmentHome Whether $HOME/.sword/ modules should be augmented
                             with the other modules.
    */
    CSwordBackend(const QString & path, const bool augmentHome = true);

    ~CSwordBackend() override;

    /** \returns the CSwordBackend singleton instance (created if needed). */
    static CSwordBackend * createInstance() {
        BT_ASSERT(!m_instance);
        m_instance = new CSwordBackend();
        return m_instance;
    }

    /** \returns the singleton instance, creating it if one does not exist. */
    static CSwordBackend * instance() { return m_instance; }

    /** \brief Destroys the singleton instance, if one exists. */
    static void destroyInstance() {
        delete m_instance;
        m_instance = nullptr;
    }

    /**
      \warning You have to call initModules() first.
      \note This method is equivalent to model()->modules().
      \returns The list of modules managed by this backend.
    */
    QList<CSwordModuleInfo*> const & moduleList() const
    { return m_dataModel->moduleList(); }

    std::shared_ptr<BtBookshelfModel> model() { return m_dataModel; }

    CSwordModuleInfo * findFirstAvailableModule(CSwordModuleInfo::ModuleType type);

    std::shared_ptr<AvailableLanguagesCacheContainer const>
    availableLanguages() noexcept;

    /**
      \brief Initializes the Sword modules.
      \returns whether the initializiation was successful.
    */
    CSwordBackend::LoadError initModules(const SetupChangedReason reason);

    /**
      \brief Deinitializes and frees the modules.
      \returns whether the method succeeded.
    */
    void shutdownModules();

    /**
      \brief Sets the state of the given filter option.
      \param[in] type The filter type whose state to set.
      \param[in] state The new filter option state.
    */
    void setOption(const CSwordModuleInfo::FilterTypes type, const int state);

    void setFilterOptions(const FilterOptions & options);

    /** \returns the language for the international booknames of Sword. */
    QString booknameLanguage() const;

    /**
      \brief Sets the language for the international booknames of Sword.
      \param[in] langName The abbreviation string which should be used for the
                          Sword backend.
    */
    void setBooknameLanguage(QString const & langName);

    /**
      \brief Searches for a module with the given description.
      \param[in] description The description of the desired module.
      \returns a pointer to the desired module or NULL if not found.
    */
    CSwordModuleInfo * findModuleByDescription(const QString & description) const;

    /**
      \brief Searches for a module with the given name.
      \param[in] name The name of the desired module.
      \returns a pointer to the desired module or NULL if not found.
    */
    CSwordModuleInfo * findModuleByName(const QString & name) const;

    /**
      \brief Searches for a module with the given sword module as module().
      \param[in] swmodule The SWModule of the desired module.
      \returns a pointer to the desired module or NULL if not found.
    */
    CSwordModuleInfo * findSwordModuleByPointer(const sword::SWModule * const swmodule) const;

    /**
      \returns The global config object containing the configs of all modules
               merged together.
    */
    sword::SWConfig * getConfig() const { return m_manager.config; }

    /**
      \param[in] option The option name to return.
      \returns The name of the option given by the parameter.
    */
    static QString optionName(const CSwordModuleInfo::FilterTypes option);

    /**
      \param[in] option The configuration option name to return.
      \returns the text used for the configuration option given.
    */
    static QString configOptionName(const CSwordModuleInfo::FilterTypes option);

    /**
      \param[in] option The translated option name.
      \returns the translated name of the option given.
    */
    static QString translatedOptionName(const CSwordModuleInfo::FilterTypes option);

    /**
      \brief Reloads all Sword modules.
      \param[in] reason The reason for the reload.
    */
    void reloadModules(const SetupChangedReason reason);

    /**
      \brief Uninstalls the given modules.
      \param[in] modules The modules to uninstall.
    */
    void uninstallModules(BtConstModuleSet const & modules);

    /**
      \param[in] names The names of the modules to return.
      \returns a list of pointers to modules, created from a list of module
               names.
    */
    QList<CSwordModuleInfo*> getPointerList(const QStringList & names) const;

    /**
      \param[in] names The names of the modules to return.
      \returns a list of pointers to const modules, created from a list of
               module names.
    */
    BtConstModuleList getConstPointerList(const QStringList & names) const;

    /**
      \brief Sword prefix list.
      \returns A list of all known Sword prefix dirs
    */
    QStringList swordDirList() const;

    /**
      Deletes all indices of modules where hasIndex() returns false (because of
      wrong index version etc.) and deletes all orphaned indexes (no module
      present) if autoDeleteOrphanedIndices is true.
    */
    void deleteOrphanedIndices();

    QString prefixPath() const
    { return QString::fromLatin1(m_manager.prefixPath); }

    sword::SWMgr & raw() { return m_manager; }

Q_SIGNALS:

    void sigSwordSetupChanged(CSwordBackend::SetupChangedReason reason);

protected: // methods:

    /**
      Creates the SWModule objects using SWMgr's methods, it adds the
      necessary filters for the module format.
    */
    CSwordBackend();

    /** Reimplemented from sword::SWMgr. */

    QStringList getSharedSwordConfigFiles() const;
    QString getPrivateSwordConfigPath() const;
    QString getPrivateSwordConfigFile() const;

private: // fields:

    struct Private: public sword::SWMgr {

    // Methods:

        using sword::SWMgr::SWMgr;

        void shutdownModules();
        void reloadConfig();
        void addRenderFilters(sword::SWModule * module,
                              sword::ConfigEntMap & section) override;

    // Fields:

        Filters::GbfToHtml   m_gbfFilter;
        Filters::OsisToHtml  m_osisFilter;
        Filters::PlainToHtml m_plainFilter;
        Filters::TeiToHtml   m_teiFilter;
        Filters::ThmlToHtml  m_thmlFilter;

    } m_manager;

    // Displays:
    Rendering::CChapterDisplay m_chapterDisplay;
    Rendering::CEntryDisplay   m_entryDisplay;
    Rendering::CBookDisplay    m_bookDisplay;

    std::shared_ptr<BtBookshelfModel> const m_dataModel;
    std::shared_ptr<AvailableLanguagesCacheContainer const>
            m_availableLanguagesCache;

    static CSwordBackend * m_instance;

};
