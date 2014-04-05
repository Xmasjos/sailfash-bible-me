
#include "BibleEngine.h"
#include "BibleWorker.h"
#include "BibleText.h"
#include "BibleBook.h"
#include "BibleChapter.h"
#include "BibleVerse.h"

#include <QQmlObjectListModel>
#include <QDir>
#include <QDateTime>
#include <QStringList>
//#include <QDebug>

/*************************** ENGINE *******************************/

BibleEngine::BibleEngine (QObject * parent) : QObject (parent) {
    m_isSearching = false;
    m_isReloading = false;
    m_isFetching  = false;
    m_isLoading   = false;
    m_searchPercent = 0.0;

    m_settings = new QSettings (this);
    m_settings->setValue ("lastStart", QDateTime::currentMSecsSinceEpoch ());

    if (!m_settings->contains ("bookmarks")) {
        m_settings->setValue ("bookmarks", QStringList ());
    }

    if (!m_settings->contains ("textFontSize")) {
        m_settings->setValue ("textFontSize", 32);
    }
    m_textFontSize = m_settings->value ("textFontSize").toReal ();

    if (!m_settings->contains ("currentTextKey")) {
        m_settings->setValue ("currentTextKey", "");
    }
    m_currentTextKey = m_settings->value ("currentTextKey").toString ();

    if (!m_settings->contains ("showLocalOnly")) {
        m_settings->setValue ("showLocalOnly", false);
    }
    m_showLocalOnly = m_settings->value ("showLocalOnly").toBool ();

    if (!m_settings->contains ("currPosId")) {
        m_settings->setValue ("currPosId", "John.3.16");
    }
    m_currentPositionId = m_settings->value ("currPosId").toString ();

    QDir dataDir (QDir::homePath ());
    dataDir.mkpath (DATADIR_PATH);

    m_confMan            = new QNetworkConfigurationManager          (this);
    m_modelTexts         = QQmlObjectListModel::create<BibleText>    (this);
    m_modelBooks         = QQmlObjectListModel::create<BibleBook>    (this);
    m_modelChapters      = QQmlObjectListModel::create<BibleChapter> (this);
    m_modelVerses        = QQmlObjectListModel::create<BibleVerse>   (this);
    m_modelBookmarks     = QQmlObjectListModel::create<BibleVerse>   (this);
    m_modelSearchResults = QQmlObjectListModel::create<BibleVerse>   (this);

    m_thread = new QThread (this);
    m_worker = new BibleWorker;
    m_worker->moveToThread (m_thread);

    connect (m_confMan, &QNetworkConfigurationManager::onlineStateChanged, this,     &BibleEngine::hasConnectionChanged);
    connect (this,      &BibleEngine::showLocalOnlyChanged,                this,     &BibleEngine::saveShowLocalOnly);
    connect (this,      &BibleEngine::textFontSizeChanged,                 this,     &BibleEngine::saveTextFontSize);
    connect (m_thread,  &QThread::started,                                 m_worker, &BibleWorker::doInit);

    connect (this,      &BibleEngine::searchRequested,                     m_worker, &BibleWorker::doSearchVerse);
    connect (this,      &BibleEngine::refreshIndexRequested,               m_worker, &BibleWorker::doRefreshIndex);
    connect (this,      &BibleEngine::loadIndexRequested,                  m_worker, &BibleWorker::doLoadIndex);
    connect (this,      &BibleEngine::downloadTextRequested,               m_worker, &BibleWorker::doDownloadText);
    connect (this,      &BibleEngine::loadTextRequested,                   m_worker, &BibleWorker::doLoadText);
    connect (this,      &BibleEngine::removeTextRequested,                 m_worker, &BibleWorker::doRemoveText);
    connect (this,      &BibleEngine::loadBookRequested,                   m_worker, &BibleWorker::doLoadBook);
    connect (this,      &BibleEngine::loadChapterRequested,                m_worker, &BibleWorker::doLoadChapter);
    connect (this,      &BibleEngine::setCurrentVerseRequested,            m_worker, &BibleWorker::doNavigateToRefId);
    connect (this,      &BibleEngine::addBookmarkRequested,                m_worker, &BibleWorker::doAddBookmark);
    connect (this,      &BibleEngine::removeBookmark,                      m_worker, &BibleWorker::doRemoveBookmark);

    connect (m_worker,  &BibleWorker::textsModelLoaded,                    this,     &BibleEngine::onTextsModelLoaded);
    connect (m_worker,  &BibleWorker::booksModelLoaded,                    this,     &BibleEngine::onBooksModelLoaded);
    connect (m_worker,  &BibleWorker::chaptersModelLoaded,                 this,     &BibleEngine::onChaptersModelLoaded);
    connect (m_worker,  &BibleWorker::versesModelLoaded,                   this,     &BibleEngine::onVersesModelLoaded);
    connect (m_worker,  &BibleWorker::currentPositionChanged,              this,     &BibleEngine::update_currentPositionId);
    connect (m_worker,  &BibleWorker::searchStarted,                       this,     &BibleEngine::onSearchStarted);
    connect (m_worker,  &BibleWorker::searchResultItem,                    this,     &BibleEngine::onSearchResultItem);
    connect (m_worker,  &BibleWorker::searchFinished,                      this,     &BibleEngine::onSearchFinished);
    connect (m_worker,  &BibleWorker::searchPercentUpdated,                this,     &BibleEngine::update_searchPercent);
    connect (m_worker,  &BibleWorker::textItemUpdated,                     this,     &BibleEngine::onTextItemUpdated);
    connect (m_worker,  &BibleWorker::currentTextChanged,                  this,     &BibleEngine::update_currentTextKey);
    connect (m_worker,  &BibleWorker::loadTextStarted,                     this,     &BibleEngine::onLoadTextStarted);
    connect (m_worker,  &BibleWorker::loadTextFinished,                    this,     &BibleEngine::onLoadTextFinished);
    connect (m_worker,  &BibleWorker::refreshStarted,                      this,     &BibleEngine::onRefreshStarted);
    connect (m_worker,  &BibleWorker::refreshFinished,                     this,     &BibleEngine::onRefreshFinished);
    connect (m_worker,  &BibleWorker::bookmarkAdded,                       this,     &BibleEngine::onBookmarkAdded);
    connect (m_worker,  &BibleWorker::bookmarksLoaded,                     this,     &BibleEngine::onBookmarksLoaded);
    connect (m_worker,  &BibleWorker::bookmarkRemoved,                     this,     &BibleEngine::onBookmarkRemoved);

    m_thread->start ();
}

BibleEngine::~BibleEngine () {
    m_thread->quit ();
    m_thread->wait ();
}

bool BibleEngine::getConnection () const {
    return m_confMan->isOnline ();
}

void BibleEngine::requestIndex () {
    //qDebug () << "BibleEngine::requestIndex";
    emit refreshIndexRequested ();
}

void BibleEngine::requestText (QString langId, QString bibleId) {
    //qDebug () << "BibleEngine::requestText" << langId << bibleId;
    emit downloadTextRequested (langId, bibleId);
}

void BibleEngine::removeText (QString key) {
    //qDebug () << "BibleEngine::removeText" << key;
    emit removeTextRequested (key);
}

void BibleEngine::reloadIndex () {
    //qDebug () << "BibleEngine::reloadIndex";
    emit loadIndexRequested ();
}

void BibleEngine::searchInText (QString str) {
    //qDebug () << "BibleEngine::searchInText" << str;
    emit searchRequested (str);
}

void BibleEngine::setCurrentVerse (QString verseId, bool force) {
    //qDebug () << "BibleEngine::setCurrentVerse" << verseId << force;
    emit setCurrentVerseRequested (verseId, force);
}

void BibleEngine::addBookmark (QString verseId) {
    //qDebug () << "BibleEngine::addBookmark" << verseId;
    emit addBookmarkRequested (verseId);
}

void BibleEngine::removeBookmark (QString verseId){
    //qDebug () << "BibleEngine::removeBookmark" << verseId;
    emit removeBookmarkRequested (verseId);
}

void BibleEngine::loadText (QString key) {
    //qDebug () << "BibleEngine::loadText" << key;
    emit loadTextRequested (key);
}

void BibleEngine::loadBook (QString bookId, bool force) {
    //qDebug () << "BibleEngine::loadBook" << bookId << force;
    emit loadBookRequested (bookId, force);
}

void BibleEngine::loadChapter (QString chapterId, bool force) {
    //qDebug () << "BibleEngine::loadChapter" << chapterId << force;
    emit loadChapterRequested (chapterId, force);
}

void BibleEngine::saveTextFontSize (qreal textFontSize) {
    //qDebug () << "BibleEngine::saveTextFontSize" << textFontSize;
    m_settings->setValue ("textFontSize", textFontSize);
}

void BibleEngine::saveShowLocalOnly (bool showLocalOnly) {
    //qDebug () << "BibleEngine::saveShowLocalOnly" << showLocalOnly;
    m_settings->setValue ("showLocalOnly", showLocalOnly);
}

void BibleEngine::onTextsModelLoaded (QVariantList items) {
    //qDebug () << "BibleEngine::onTextsModelLoaded" << items;
    m_indexTexts.clear ();
    m_modelTexts->clear ();
    foreach (QVariant variant, items) {
        QVariantMap item = variant.toMap ();
        BibleText * text = BibleText::fromQtVariant (item);
        m_modelTexts->append (text);

        QString langId  = item.value ("languageID").toString ();
        QString bibleId = item.value ("bibleID").toString ();
        QString key = QString ("%1__%2").arg (langId).arg (bibleId);
        m_indexTexts.insert (key, text);
    }
}

void BibleEngine::onBooksModelLoaded (QVariantList items) {
    //qDebug () << "BibleEngine::onBooksModelLoaded" << items.count ();
    m_modelBooks->clear ();
    foreach (QVariant variant, items) {
        BibleBook * book = BibleBook::fromQtVariant (variant.toMap ());
        m_modelBooks->append (book);
    }
}

void BibleEngine::onChaptersModelLoaded (QVariantList items) {
    //qDebug () << "BibleEngine::onChaptersModelLoaded" << items.count ();
    m_modelChapters->clear ();
    foreach (QVariant variant, items) {
        BibleChapter * chapter = BibleChapter::fromQtVariant (variant.toMap ());
        m_modelChapters->append (chapter);
    }
}

void BibleEngine::onVersesModelLoaded (QVariantList items) {
    //qDebug () << "BibleEngine::onVersesModelLoaded" << items.count ();
    m_indexVerses.clear ();
    m_modelVerses->clear ();
    foreach (QVariant variant, items) {
        BibleVerse * verse = BibleVerse::fromQtVariant (variant.toMap ());
        m_modelVerses->append (verse);
        m_indexVerses.insert (verse->get_verseId (), verse);
    }
}

void BibleEngine::onSearchStarted () {
    update_isSearching (true);
    m_modelSearchResults->clear ();
}

void BibleEngine::onSearchResultItem (QVariantMap verse) {
    m_modelSearchResults->append (BibleVerse::fromQtVariant (verse));
}

void BibleEngine::onSearchFinished () {
    update_isSearching (false);
}

void BibleEngine::onBookmarksLoaded (QVariantList items) {
    //qDebug () << "BibleEngine::onBookmarksLoaded" << items.count ();
    m_modelBookmarks->clear ();
    foreach (QVariant variant, items) {
        BibleVerse * bookmark = BibleVerse::fromQtVariant (variant.toMap ());
        m_modelBookmarks->append (bookmark);
        BibleVerse * verse = m_indexVerses.value (bookmark->get_verseId (), NULL);
        if (verse) {
            verse->update_marked (true);
        }
    }
}

void BibleEngine::onBookmarkAdded (QVariantMap item) {
    //qDebug () << "BibleEngine::onBookmarkAdded" << item;
    BibleVerse * bookmark = BibleVerse::fromQtVariant (item);
    m_modelBookmarks->append (bookmark);
    BibleVerse * verse = m_indexVerses.value (bookmark->get_verseId (), NULL);
    if (verse) {
        verse->update_marked (true);
    }
}

void BibleEngine::onBookmarkRemoved (QString verseId) {
    //qDebug () << "BibleEngine::onBookmarkRemoved" << verseId;
    for (int idx = 0; idx < m_modelBookmarks->count (); idx++) {
        if (m_modelBookmarks->get (idx)->property ("verseId") == verseId) {
            m_modelBookmarks->remove (idx);
            break;
        }
    }
    BibleVerse * verse = m_indexVerses.value (verseId, NULL);
    if (verse) {
        verse->update_marked (false);
    }
}

void BibleEngine::onLoadTextStarted () {
    update_isLoading (true);
}

void BibleEngine::onLoadTextFinished () {
    update_isLoading (false);
}

void BibleEngine::onRefreshStarted () {
    update_isFetching (true);
}

void BibleEngine::onRefreshFinished () {
    update_isFetching (false);
}

void BibleEngine::onTextItemUpdated (QString textKey, QVariantMap item) {
    BibleText * text = m_indexTexts.value (textKey, NULL);
    if (text) {
        text->updateWithQtVariant (item);
    }
}

