/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
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



#ifndef KT_WEBVIEW_H
#define KT_WEBVIEW_H

#include <KWebView>
#include <QNetworkReply>


namespace kt
{
	
	class SearchUrlBuilder
	{
	public:
		virtual ~SearchUrlBuilder() {}
		
		virtual KUrl searchUrl(const QString & search_text) = 0;
	};
	
	/**
		WebView provides a webkit view which supports for the ktorrent homepage.
	 */
	class WebView : public KWebView
	{
		Q_OBJECT
	public:
		WebView(SearchUrlBuilder* search_url_builder, QWidget* parentWidget = 0);
		virtual ~WebView();
		
		/**
		 * Open a url
		 * @param url The KUrl
		 */
		void openUrl(const KUrl & url);
		
		/**
		 * Show the home page
		 */
		void home();
		
		/**
		 * Get a search url for a search text
		 * @param search_text The text to search
		 * @return A KUrl to load
		 */
		KUrl searchUrl(const QString & search_text);
		
	protected:
		void loadHomePage();
		
	private:
		QString home_page_html;
		QString home_page_base_url;
		SearchUrlBuilder* search_url_builder;
	};

}

#endif // KT_HOMEPAGE_H
