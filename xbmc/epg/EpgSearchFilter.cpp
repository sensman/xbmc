/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "guilib/LocalizeStrings.h"
#include "utils/TextSearch.h"
#include "utils/log.h"
#include "FileItem.h"
#include "../addons/include/xbmc_pvr_types.h"

#include "EpgSearchFilter.h"
#include "EpgContainer.h"

using namespace std;
using namespace EPG;

void EpgSearchFilter::Reset()
{
  m_strSearchTerm            = "";
  m_bIsCaseSensitive         = false;
  m_bSearchInDescription     = false;
  m_iGenreType               = EPG_SEARCH_UNSET;
  m_iGenreSubType            = EPG_SEARCH_UNSET;
  m_iMinimumDuration         = EPG_SEARCH_UNSET;
  m_iMaximumDuration         = EPG_SEARCH_UNSET;
  m_startDateTime            = g_EpgContainer.GetFirstEPGDate();
  m_endDateTime              = g_EpgContainer.GetLastEPGDate();
  m_bIncludeUnknownGenres    = false;
  m_bIgnorePresentTimers     = false;
  m_bIgnorePresentRecordings = false;
  m_bPreventRepeats          = false;
}

bool EpgSearchFilter::MatchGenre(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iGenreType != EPG_SEARCH_UNSET)
  {
    bool bIsUnknownGenre(tag.GenreType() > EPG_EVENT_CONTENTMASK_USERDEFINED ||
        tag.GenreType() < EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
    bReturn = ((m_bIncludeUnknownGenres && bIsUnknownGenre) || tag.GenreType() == m_iGenreType);
  }

  return bReturn;
}

bool EpgSearchFilter::MatchDuration(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (m_iMinimumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag.GetDuration() > m_iMinimumDuration * 60);

  if (bReturn && m_iMaximumDuration != EPG_SEARCH_UNSET)
    bReturn = (tag.GetDuration() < m_iMaximumDuration * 60);

  return bReturn;
}

bool EpgSearchFilter::MatchStartAndEndTimes(const CEpgInfoTag &tag) const
{
  return (tag.StartAsLocalTime() >= m_startDateTime && tag.EndAsLocalTime() <= m_endDateTime);
}

bool EpgSearchFilter::MatchSearchTerm(const CEpgInfoTag &tag) const
{
  bool bReturn(true);

  if (!m_strSearchTerm.IsEmpty())
  {
    cTextSearch search(tag.Title(), m_strSearchTerm, m_bIsCaseSensitive);
    bool bTitleMatch = search.DoSearch();

    search.SetText(tag.PlotOutline(), m_strSearchTerm, m_bIsCaseSensitive);
    bReturn = bTitleMatch || (m_bSearchInDescription && search.DoSearch());
  }

  return bReturn;
}

bool EpgSearchFilter::FilterEntry(const CEpgInfoTag &tag) const
{
  return (MatchGenre(tag) &&
      MatchDuration(tag) &&
      MatchStartAndEndTimes(tag) &&
      MatchSearchTerm(tag));
}

int EpgSearchFilter::RemoveDuplicates(CFileItemList *results)
{
  unsigned int iSize = results->Size();

  for (unsigned int iResultPtr = 0; iResultPtr < iSize; iResultPtr++)
  {
    const CEpgInfoTag *epgentry_1 = results->Get(iResultPtr)->GetEPGInfoTag();
    for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
    {
      const CEpgInfoTag *epgentry_2 = results->Get(iTagPtr)->GetEPGInfoTag();
      if (iResultPtr == iTagPtr)
        continue;

      if (epgentry_1->Title()       != epgentry_2->Title() ||
          epgentry_1->Plot()        != epgentry_2->Plot() ||
          epgentry_1->PlotOutline() != epgentry_2->PlotOutline())
        continue;

      results->Remove(iTagPtr);
      iResultPtr--;
      iTagPtr--;
      iSize--;
    }
  }

  return iSize;
}
