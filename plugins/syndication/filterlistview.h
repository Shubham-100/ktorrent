/***************************************************************************
 *   Copyright (C) 2008 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
 *   ivasic@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef KTFILTERLISTVIEW_H
#define KTFILTERLISTVIEW_H

#include <QListView>

namespace kt
{
    class Filter;
    class FilterList;

    /**
        List view to display filters.
    */
    class FilterListView : public QListView
    {
        Q_OBJECT
    public:
        FilterListView(FilterList* filters, QWidget* parent);
        ~FilterListView();

        QModelIndexList selectedFilters();

        void itemActivated(const QModelIndex& idx);
        void selectionChanged(const QItemSelection& sel, const QItemSelection& desel);

    Q_SIGNALS:
        void filterActivated(Filter* filter);
        void enableRemove(bool on);
        void enableEdit(bool on);

    private:
        FilterList* filters;
    };

}

#endif
