// qtractorConnectForm.h
//
/****************************************************************************
   Copyright (C) 2005-2006, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qtractorConnectForm_h
#define __qtractorConnectForm_h

#include "ui_qtractorConnectForm.h"


#include "qtractorAudioConnect.h"
#include "qtractorMidiConnect.h"


// Forward declarations...
class qtractorSession;
class qtractorOptions;


//----------------------------------------------------------------------------
// qtractorConnectForm -- UI wrapper form.

class qtractorConnectForm : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qtractorConnectForm(QWidget *pParent = 0, Qt::WFlags wflags = 0);
	// Destructor.
	~qtractorConnectForm();

    void setSession(qtractorSession *pSession);
    qtractorSession *session();
    void setOptions(qtractorOptions *pOptions);
    qtractorOptions *options();

	QTabWidget *connectTabWidget() const
		{ return m_ui.ConnectTabWidget; }

	QSplitter *audioConnectSplitter() const
		{ return m_ui.AudioConnectSplitter; }
	QSplitter *midiConnectSplitter() const
		{ return m_ui.AudioConnectSplitter; }

	QComboBox *audioOClientsComboBox() const
		{ return m_ui.AudioOClientsComboBox; }
	QComboBox *audioIClientsComboBox() const
		{ return m_ui.AudioIClientsComboBox; }
	qtractorClientListView *audioOListView() const
		{ return m_ui.AudioOListView; }
	qtractorClientListView *audioIListView() const
		{ return m_ui.AudioIListView; }

	QComboBox *midiOClientsComboBox() const
		{ return m_ui.MidiOClientsComboBox; }
	QComboBox *midiIClientsComboBox() const
		{ return m_ui.MidiIClientsComboBox; }
	qtractorClientListView *midiOListView() const
		{ return m_ui.MidiOListView; }
	qtractorClientListView *midiIListView() const
		{ return m_ui.MidiIListView; }

public slots:

	void audioIClientChanged();
	void audioOClientChanged();
    void audioConnectSelected();
    void audioDisconnectSelected();
    void audioDisconnectAll();
    void audioRefresh();
    void audioStabilize();
	void midiIClientChanged();
	void midiOClientChanged();
    void midiConnectSelected();
    void midiDisconnectSelected();
    void midiDisconnectAll();
    void midiRefresh();
    void midiStabilize();

protected:

    void updateClientsComboBox(QComboBox *pComboBox,
		qtractorClientListView *pClientListView, const QIcon& icon);

private:

	// The Qt-designer UI struct...
	Ui::qtractorConnectForm m_ui;

	// Instance variables...
	qtractorSession *m_pSession;
	qtractorOptions *m_pOptions;
	qtractorAudioConnect *m_pAudioConnect;
	qtractorMidiConnect *m_pMidiConnect;
};


#endif	// __qtractorConnectForm_h


// end of qtractorConnectForm.h
