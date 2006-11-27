#pragma once
#include "Database.h"
#include "utils\IMDB.h"
#include "settings/VideoSettings.h"

typedef vector<CStdString> VECMOVIEYEARS;
typedef vector<CStdString> VECMOVIEACTORS;
typedef vector<CStdString> VECMOVIEGENRES;
typedef vector<CIMDBMovie> VECMOVIES;
typedef vector<CStdString> VECMOVIESFILES;

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2


class CBookmark
{
public:
  CBookmark();
  double timeInSeconds;
  CStdString thumbNailImage;
  CStdString playerState;
  CStdString player;

  enum EType
  {
    STANDARD = 0,
    RESUME = 1,
  } type;
};

typedef vector<CBookmark> VECBOOKMARKS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%


class CVideoDatabase : public CDatabase
{
public:
  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);

  long AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles);

  void MarkAsWatched(const CFileItem &item);
  void MarkAsWatched(long lMovieId);

  void MarkAsUnWatched(long lMovieId);
  void UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle);

  void GetGenres(VECMOVIEGENRES& genres, int iShowMode = VIDEO_SHOW_ALL);
  void GetMoviesByGenre(CStdString& strGenre, VECMOVIES& movies);

  void GetActors(VECMOVIEACTORS& actors, int iShowMode = VIDEO_SHOW_ALL);
  void GetMoviesByActor(CStdString& strActor, VECMOVIES& movies);

  void GetYears(VECMOVIEYEARS& years, int iShowMode = VIDEO_SHOW_ALL);
  void GetMoviesByYear(CStdString& strYear, VECMOVIES& movies);

  bool HasMovieInfo(const CStdString& strFilenameAndPath);
  bool HasSubtitle(const CStdString& strFilenameAndPath);
  void DeleteMovieInfo(const CStdString& strFileNameAndPath);

  void GetMovies(VECMOVIES& movies);
  void GetFilePath(long lMovieId, CStdString &filePath);
  void GetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lMovieId = -1);
  long GetMovieInfo(const CStdString& strFilenameAndPath);
  void SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details);
  void GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies);

  void GetBookMarksForMovie(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD);
  void AddBookMarkToMovie(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark);
  void ClearBookMarkOfVideo(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfMovie(const CStdString& strFilenameAndPath, CBookmark::EType type = CBookmark::STANDARD);

  void DeleteMovie(const CStdString& strFilenameAndPath);
  void GetDVDLabel(long lMovieId, CStdString& strDVDLabel);
  void SetDVDLabel(long lMovieId, const CStdString& strDVDLabel1);
  int GetRecentMovies(long* pMovieIdArray, int nSize);

  bool GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CVideoSettings &settings);
  void EraseVideoSettings();

  bool GetStackTimes(const CStdString &filePath, vector<long> &times);
  void SetStackTimes(const CStdString &filePath, vector<long> &times);

  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, long idYear=-1);
  bool GetActorsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetGenreById(long lIdGenre, CStdString& strGenre);
  int GetMovieCount();

  void CleanDatabase();

protected:
  long GetPath(const CStdString& strPath);
  long AddPath(const CStdString& strPath);

  long GetFile(const CStdString& strFilenameAndPath, long &lPathId, long& lMovieId, bool bExact = false);
  long AddFile(long lMovieId, long lPathId, const CStdString& strFileName);

  long GetMovie(const CStdString& strFilenameAndPath);

  long AddGenre(const CStdString& strGenre1);
  long AddActor(const CStdString& strActor);

  void AddActorToMovie(long lMovieId, long lActorId);
  void AddGenreToMovie(long lMovieId, long lGenreId);

  CIMDBMovie GetDetailsFromDataset(auto_ptr<Dataset> &pDS);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

};
