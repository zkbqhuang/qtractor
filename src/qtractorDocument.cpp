// qtractorDocument.cpp
//
/****************************************************************************
   Copyright (C) 2005-2010, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qtractorDocument.h"

#include <QDomDocument>

#include <QFileInfo>
#include <QTextStream>


//-------------------------------------------------------------------------
// qtractorDocument -- Session file import/export helper class.
//

// Constructor.
qtractorDocument::qtractorDocument ( QDomDocument *pDocument,
	const QString& sTagName, bool bTemplate )
{
	m_pDocument = pDocument;
	m_sTagName  = sTagName;
	m_bTemplate = bTemplate;
}

// Default destructor.
qtractorDocument::~qtractorDocument (void)
{
}


//-------------------------------------------------------------------------
// qtractorDocument -- accessors.
//

QDomDocument *qtractorDocument::document (void) const
{
	return m_pDocument;
}


// Template mode property.
void qtractorDocument::setTemplate ( bool bTemplate )
{
	m_bTemplate = bTemplate;
}

bool qtractorDocument::isTemplate (void) const
{
	return m_bTemplate;
}


void qtractorDocument::saveTextElement ( const QString& sTagName,
	const QString& sText, QDomElement *pElem )
{
    QDomElement eTag = m_pDocument->createElement(sTagName);
    eTag.appendChild(m_pDocument->createTextNode(sText));
	pElem->appendChild(eTag);
}


//-------------------------------------------------------------------------
// qtractorDocument -- loaders.
//

// External storage simple load method.
bool qtractorDocument::load ( const QString& sFilename, bool bTemplate )
{
	// Hold template mode.
	setTemplate(bTemplate);

	// Open file...
	QFile file(sFilename);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	// Parse it a-la-DOM :-)
	if (!m_pDocument->setContent(&file)) {
		file.close();
		return false;
	}
	file.close();

	// Get root element and check for proper taqg name.
	QDomElement elem = m_pDocument->documentElement();
	if (elem.tagName() != m_sTagName)
	    return false;

	return loadElement(&elem);
}


//-------------------------------------------------------------------------
// qtractorDocument -- savers.
//

bool qtractorDocument::save ( const QString& sFilename, bool bTemplate )
{
	// Hold template mode.
	setTemplate(bTemplate);

	// We must have a valid tag name...
	if (m_sTagName.isEmpty())
		return false;

	// Save spec...
	QDomElement elem = m_pDocument->createElement(m_sTagName);
	if (!saveElement(&elem))
		return false;
	m_pDocument->appendChild(elem);

	// Finally, we're ready to save to external file.
	QFile file(sFilename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	QTextStream ts(&file);
	ts << m_pDocument->toString() << endl;
	file.close();

	return true;
}


//-------------------------------------------------------------------------
// qtractorDocument -- helpers.
//

bool qtractorDocument::boolFromText ( const QString& sText )
{
	return (sText == "true" || sText == "on" || sText == "yes" || sText == "1");
}

QString qtractorDocument::textFromBool ( bool bBool )
{
	return QString::number(bBool ? 1 : 0);
}


// end of qtractorDocument.cpp
