// qtractorPluginFactory.cpp
//
/****************************************************************************
   Copyright (C) 2005-2017, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qtractorAbout.h"
#include "qtractorPluginFactory.h"

#ifdef CONFIG_LADSPA
#include "qtractorLadspaPlugin.h"
#endif
#ifdef CONFIG_DSSI
#include "qtractorDssiPlugin.h"
#endif
#ifdef CONFIG_VST
#include "qtractorVstPlugin.h"
#endif
#ifdef CONFIG_LV2
#include "qtractorLv2Plugin.h"
#endif

#include "qtractorInsertPlugin.h"

#include "qtractorOptions.h"

#include <QApplication>

#include <QTextStream>
#include <QFileInfo>
#include <QDir>

#if QT_VERSION < 0x050000
#include <QDesktopServices>
#else
#include <QStandardPaths>
#endif


//----------------------------------------------------------------------------
// qtractorPluginFactory -- Plugin path helper.
//

// Singleton instance pointer.
qtractorPluginFactory *qtractorPluginFactory::g_pPluginFactory = NULL;

// Singleton instance accessor (static).
qtractorPluginFactory *qtractorPluginFactory::getInstance (void)
{
	return g_pPluginFactory;
}


// Contructor.
qtractorPluginFactory::qtractorPluginFactory ( QObject *pParent )
	: QObject(pParent), m_typeHint(qtractorPluginType::Any), m_pProxy(NULL)
{
	g_pPluginFactory = this;
}


// Destructor.
qtractorPluginFactory::~qtractorPluginFactory (void)
{
	g_pPluginFactory = NULL;

	reset();
	clear();

	m_paths.clear();
}



// A common scheme for (a default) plugin serach paths...
//
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
#define PATH_SEP ";"
#else
#define PATH_SEP ":"
#endif

static QString default_paths ( const QString& suffix )
{
	const QString& sep  = QDir::separator();

	const QString& home = QDir::homePath();

	const QString& pre1 = QDir::rootPath() + "usr";
	const QString& pre2 = pre1 + sep + "local";

	const QString& lib0 = "lib";
	const QString& lib1 = pre1 + sep + lib0;
	const QString& lib2 = pre2 + sep + lib0;

#if defined(__x86_64__)
	const QString& x64  = "64";
	const QString& lib3 = lib1 + x64;
	const QString& lib4 = lib2 + x64;
#endif

	QStringList paths;

	paths << home + sep + '.' + suffix;

#if defined(__x86_64__)
//	paths << home + sep + lib0 + x64 + sep + suffix;
	paths << lib4 + sep + suffix;
	paths << lib3 + sep + suffix;
#endif

//	paths << home + sep + lib0 + sep + suffix;
	paths << lib2 + sep + suffix;
	paths << lib1 + sep + suffix;

	return paths.join(PATH_SEP);
}


void qtractorPluginFactory::updatePluginPaths (void)
{
	m_paths.clear();

	qtractorOptions *pOptions = qtractorOptions::getInstance();

#ifdef CONFIG_LADSPA
	// LADSPA default path...
	QStringList ladspa_paths;
	if (pOptions)
		ladspa_paths = pOptions->ladspaPaths;
	if (ladspa_paths.isEmpty()) {
		QString sLadspaPaths = ::getenv("LADSPA_PATH");
		if (sLadspaPaths.isEmpty())
			sLadspaPaths = default_paths("ladspa");
		ladspa_paths = sLadspaPaths.split(PATH_SEP);
	}
	m_paths.insert(qtractorPluginType::Ladspa, ladspa_paths);
#endif

#ifdef CONFIG_DSSI
	// DSSI default path...
	QStringList dssi_paths;
	if (pOptions)
		dssi_paths = pOptions->dssiPaths;
	if (dssi_paths.isEmpty()) {
		QString sDssiPaths = ::getenv("DSSI_PATH");
		if (sDssiPaths.isEmpty())
			sDssiPaths = default_paths("dssi");
		dssi_paths = sDssiPaths.split(PATH_SEP);
	}
	m_paths.insert(qtractorPluginType::Dssi, dssi_paths);
#endif

#ifdef CONFIG_VST
	// VST default path...
	QStringList vst_paths;
	if (pOptions)
		vst_paths = pOptions->vstPaths;
	if (vst_paths.isEmpty()) {
		QString sVstPaths = ::getenv("VST_PATH");
		if (sVstPaths.isEmpty())
			sVstPaths = default_paths("vst");
		vst_paths = sVstPaths.split(PATH_SEP);
	}
	m_paths.insert(qtractorPluginType::Vst, vst_paths);
#endif

#ifdef CONFIG_LV2
	// LV2 default path...
	QStringList lv2_paths;
	if (pOptions)
		lv2_paths = pOptions->lv2Paths;
	if (lv2_paths.isEmpty()) {
		QString sLv2Paths = ::getenv("LV2_PATH");
		if (sLv2Paths.isEmpty())
			sLv2Paths = default_paths("lv2");
		lv2_paths = sLv2Paths.split(PATH_SEP);
	}
#ifdef CONFIG_LV2_PRESETS
	QString sLv2PresetDir;
	if (pOptions)
		sLv2PresetDir = pOptions->sLv2PresetDir;
	if (sLv2PresetDir.isEmpty())
		sLv2PresetDir = QDir::homePath() + QDir::separator() + ".lv2";
	if (!lv2_paths.contains(sLv2PresetDir))
		lv2_paths.append(sLv2PresetDir);
#endif
	m_paths.insert(qtractorPluginType::Lv2, lv2_paths);
	// HACK: set special environment for LV2...
	::setenv("LV2_PATH", lv2_paths.join(PATH_SEP).toUtf8().constData(), 1);
#endif
}


QStringList qtractorPluginFactory::pluginPaths (
	qtractorPluginType::Hint typeHint )
{
	if (m_paths.isEmpty()) // Just in case...
		updatePluginPaths();

	return m_paths.value(typeHint);
}


// Executive methods.
void qtractorPluginFactory::scan (void)
{
	// Start clean.
	reset();

	if (m_paths.isEmpty()) // Just in case...
		updatePluginPaths();

	// Get paths based on hints...
	int iFileCount = 0;

#ifdef CONFIG_LADSPA
	// LADSPA default path...
	if (m_typeHint == qtractorPluginType::Any ||
		m_typeHint == qtractorPluginType::Ladspa) {
		const QStringList& paths = m_paths.value(qtractorPluginType::Ladspa);
		if (!paths.isEmpty())
			iFileCount += addFiles(qtractorPluginType::Ladspa, paths);
	}
#endif
#ifdef CONFIG_DSSI
	// DSSI default path...
	if (m_typeHint == qtractorPluginType::Any ||
		m_typeHint == qtractorPluginType::Dssi) {
		const QStringList& paths = m_paths.value(qtractorPluginType::Dssi);
		if (!paths.isEmpty())
			iFileCount += addFiles(qtractorPluginType::Dssi, paths);
	}
#endif
#ifdef CONFIG_VST
	// VST default path...
	if (m_typeHint == qtractorPluginType::Any ||
		m_typeHint == qtractorPluginType::Vst) {
		const QStringList& paths = m_paths.value(qtractorPluginType::Vst);
		if (!paths.isEmpty()) {
			iFileCount += addFiles(qtractorPluginType::Vst, paths);
			qtractorOptions *pOptions = qtractorOptions::getInstance();
			if (pOptions && pOptions->bDummyVstScan) {
				const int iDummyVstHash
					= m_files.value(qtractorPluginType::Vst).count();
				m_pProxy = new qtractorPluginFactoryProxy(this);
				m_pProxy->open(iDummyVstHash != pOptions->iDummyVstHash);
				pOptions->iDummyVstHash = iDummyVstHash;
			}
		}
	}
#endif
#ifdef CONFIG_LV2
	// LV2 default path...
	if (m_typeHint == qtractorPluginType::Any ||
		m_typeHint == qtractorPluginType::Lv2) {
		QStringList& files = m_files[qtractorPluginType::Lv2];
		files.append(qtractorLv2PluginType::lv2_plugins());
		iFileCount += files.count();
	}
#endif

	// Do the real scan...
	int iFile = 0;
	Paths::ConstIterator files_iter = m_files.constBegin();
	const Paths::ConstIterator& files_end = m_files.constEnd();
	for ( ; files_iter != files_end; ++files_iter) {
		const qtractorPluginType::Hint typeHint = files_iter.key();
		QStringListIterator file_iter(files_iter.value());
		while (file_iter.hasNext()) {
			addTypes(typeHint, file_iter.next());
			emit scanned((++iFile * 100) / iFileCount);
			QApplication::processEvents(
				QEventLoop::ExcludeUserInputEvents);
		}
	}

	// Check the proxy (out-of-process) client closure...
	if (m_pProxy)
		m_pProxy->close();

	// Done.
	reset();
}


void qtractorPluginFactory::reset (void)
{
	if (m_pProxy) {
		m_pProxy->terminate();
		delete m_pProxy;
		m_pProxy = NULL;
	}

	m_files.clear();
}


void qtractorPluginFactory::clear (void)
{
	qDeleteAll(m_types);
	m_types.clear();
}


void qtractorPluginFactory::clearAll (void)
{
	QFile::remove(qtractorPluginFactoryProxy::cacheFilePath());

	clear();
}


// Recursive plugin file/path inventory method.
int qtractorPluginFactory::addFiles (
	qtractorPluginType::Hint typeHint, const QStringList& paths )
{
	int iFileCount = 0;

	QStringListIterator path_iter(paths);
	while (path_iter.hasNext())
		iFileCount += addFiles(typeHint, path_iter.next());

	return iFileCount;
}

int qtractorPluginFactory::addFiles (
	qtractorPluginType::Hint typeHint, const QString& sPath )
{
	int iFileCount = 0;

	const QDir dir(sPath);
	QDir::Filters filters = QDir::Files;
	if (typeHint == qtractorPluginType::Vst)
		filters = filters | QDir::AllDirs | QDir::NoDotAndDotDot;
	const QFileInfoList& info_list = dir.entryInfoList(filters);
	QListIterator<QFileInfo> info_iter(info_list);
	while (info_iter.hasNext()) {
		const QFileInfo& info = info_iter.next();
		const QString& sFilename = info.absoluteFilePath();
		if (info.isDir() && info.isReadable())
			iFileCount += addFiles(typeHint, sFilename);
		else
		if (QLibrary::isLibrary(sFilename)) {
			m_files[typeHint].append(sFilename);
			++iFileCount;
		}
	}

	return iFileCount;
}


// Plugin factory method (static).
qtractorPlugin *qtractorPluginFactory::createPlugin (
	qtractorPluginList *pList,
	const QString& sFilename, unsigned long iIndex,
	qtractorPluginType::Hint typeHint )
{
#ifdef CONFIG_DEBUG
	qDebug("qtractorPluginFactory::createPlugin(%p, \"%s\", %lu, %d)",
		pList, sFilename.toUtf8().constData(), iIndex, int(typeHint));
#endif

	// Attend to insert pseudo-plugin hints...
	if (sFilename.isEmpty()) {
		if (typeHint == qtractorPluginType::Insert)
			return qtractorInsertPluginType::createPlugin(pList, iIndex);
		else
		if (typeHint == qtractorPluginType::AuxSend)
			return qtractorAuxSendPluginType::createPlugin(pList, iIndex);
		else
		// Don't bother with anything else.
		return NULL;
	}

#ifdef CONFIG_LV2
	// Try LV2 plugins hints before anything else...
	if (typeHint == qtractorPluginType::Lv2) {
		qtractorLv2PluginType *pLv2Type
			= qtractorLv2PluginType::createType(sFilename);
		if (pLv2Type) {
			if (pLv2Type->open())
				return new qtractorLv2Plugin(pList, pLv2Type);
			delete pLv2Type;
		}
		// Bail out.
		return NULL;
	}
#endif

	// Try to fill the types list at this moment...
	qtractorPluginFile *pFile = qtractorPluginFile::addFile(sFilename);
	if (pFile == NULL)
		return NULL;

#ifdef CONFIG_DSSI
	// Try DSSI plugin types first...
	if (typeHint == qtractorPluginType::Dssi) {
		qtractorDssiPluginType *pDssiType
			= qtractorDssiPluginType::createType(pFile, iIndex);
		if (pDssiType) {
			pFile->addRef();
			if (pDssiType->open())
				return new qtractorDssiPlugin(pList, pDssiType);
			delete pDssiType;
		}
	}
#endif

#ifdef CONFIG_LADSPA
	// Try LADSPA plugin types...
	if (typeHint == qtractorPluginType::Ladspa) {
		qtractorLadspaPluginType *pLadspaType
			= qtractorLadspaPluginType::createType(pFile, iIndex);
		if (pLadspaType) {
			pFile->addRef();
			if (pLadspaType->open())
				return new qtractorLadspaPlugin(pList, pLadspaType);
			delete pLadspaType;
		}
	}
#endif

#ifdef CONFIG_VST
	// Try VST plugin types...
	if (typeHint == qtractorPluginType::Vst) {
		qtractorVstPluginType *pVstType
			= qtractorVstPluginType::createType(pFile, iIndex);
		if (pVstType) {
			pFile->addRef();
			if (pVstType->open())
				return new qtractorVstPlugin(pList, pVstType);
			delete pVstType;
		}
	}
#endif

	// Bad luck, no valid plugin found...
	qtractorPluginFile::removeFile(pFile);

	return NULL;
}


// Plugin type listing.
bool qtractorPluginFactory::addTypes (
	qtractorPluginType::Hint typeHint, const QString& sFilename )
{
	// Try to fill the types list at this moment...
	qtractorPluginType *pType;

#ifdef CONFIG_LV2
	// Try LV2 plugin types...
	if (typeHint == qtractorPluginType::Lv2) {
		pType = qtractorLv2PluginType::createType(sFilename);
		if (pType) {
			if (pType->open()) {
				addType(pType);
				pType->close();
				return true;
			} else {
				delete pType;
			}
		}
		// None found.
		return false;
	}
#endif

#ifdef CONFIG_VST
	// Try VST plugin types (out-of-process scan)...
	if (typeHint == qtractorPluginType::Vst && m_pProxy)
		return m_pProxy->addTypes(typeHint, sFilename);
#endif

	qtractorPluginFile *pFile = qtractorPluginFile::addFile(sFilename);
	if (pFile == NULL)
		return false;

	unsigned long iIndex = 0;

#ifdef CONFIG_DSSI
	// Try DSSI plugin types...
	if (typeHint == qtractorPluginType::Dssi) {
		while (true) {
			pType = qtractorDssiPluginType::createType(pFile, iIndex);
			if (pType == NULL)
				break;
			if (pType->open()) {
				pFile->addRef();
				addType(pType);
				pType->close();
				++iIndex;
			} else {
				delete pType;
				break;
			}
		}
		// Have we found some, already?
		if (iIndex > 0) {
			pFile->close();
			return true;
		}
	}
#endif

#ifdef CONFIG_LADSPA
	// Try LADSPA plugin types...
	if (typeHint == qtractorPluginType::Ladspa) {
		while (true) {
			pType = qtractorLadspaPluginType::createType(pFile, iIndex);
			if (pType == NULL)
				break;
			if (pType->open()) {
				pFile->addRef();
				addType(pType);
				pType->close();
				++iIndex;
			} else {
				delete pType;
				break;
			}
		}
		// Have we found some, already?
		if (iIndex > 0) {
			pFile->close();
			return true;
		}
	}
#endif

#ifdef CONFIG_VST
	// Try VST plugin types...
	if (typeHint == qtractorPluginType::Vst) {
		while (true) {
			pType = qtractorVstPluginType::createType(pFile, iIndex);
			if (pType == NULL)
				break;
			if (pType->open()) {
				pFile->addRef();
				addType(pType);
				pType->close();
				++iIndex;
			} else {
				delete pType;
				break;
			}
		}
		// Have we found some, already?
		if (iIndex > 0) {
			pFile->close();
			return true;
		}
	}
#endif

	// We probably have nothing here.
	qtractorPluginFile::removeFile(pFile);

	return false;
}


//----------------------------------------------------------------------------
// qtractorPluginFactoryProxy -- Plugin path proxy (out-of-process client).
//

// Constructor.
qtractorPluginFactoryProxy::qtractorPluginFactoryProxy (
	qtractorPluginFactory *pPluginFactory )
	: QProcess(pPluginFactory), m_iFileCount(0), m_iExitStatus(-1)
{
	QObject::connect(this,
		SIGNAL(readyReadStandardOutput()),
		SLOT(stdout_slot()));
	QObject::connect(this,
		SIGNAL(readyReadStandardError()),
		SLOT(stderr_slot()));
	QObject::connect(this,
		SIGNAL(finished(int, QProcess::ExitStatus)),
		SLOT(exit_slot(int, QProcess::ExitStatus)));
}


// Open/start method.
bool qtractorPluginFactoryProxy::open ( bool bReset )
{
	// Cache file setup...
	m_file.setFileName(cacheFilePath());
	m_list.clear();

	// Open and read cache file, whether applicable...
	if (!bReset && m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		// Read from cache...
		QTextStream sin(&m_file);
		while (!sin.atEnd()) {
			const QString& sText = sin.readLine();
			if (sText.isEmpty())
				continue;
			const QStringList& props = sText.split('|');
			if (props.count() >= 6) // get filename...
				m_list[props.at(6)].append(sText);
		}
		// May close the file.
		m_file.close();
		return true;
	}

	// Make sure cache file location do exists...
	const QFileInfo fi(m_file);
	if (!fi.dir().mkpath(fi.absolutePath()))
		return false;

	// Open cache file for writing...
	if (!m_file.open(QIODevice::Append | QIODevice::Text | QIODevice::Truncate))
		return false;

	// Go go go...
	return start();
}


// Close/stop method.
void qtractorPluginFactoryProxy::close (void)
{
	// We're we scanning hard?...
	if (QProcess::state() != QProcess::NotRunning) {
		QProcess::closeWriteChannel();
		for (int iFile = 0; iFile < m_iFileCount; ++iFile) {
			if (QProcess::waitForFinished(200) || m_iExitStatus >= 0)
				break;
			QApplication::processEvents(
				QEventLoop::ExcludeUserInputEvents);
		}
	}

	// Close cache file...
	if (m_file.isOpen())
		m_file.close();

	// Cleanup cache...
	m_list.clear();
}


// Scan start method.
bool qtractorPluginFactoryProxy::start (void)
{
	// Maybe we're still running, doh!
	if (QProcess::state() != QProcess::NotRunning)
		return false;

	// Start from scratch...
	m_iFileCount = 0;
	m_iExitStatus = -1;

	// Get the main scanner executable...
	const QDir dir(QApplication::applicationDirPath());
	const QFileInfo fi(dir, "qtractor_vst_scan");
	if (!fi.isExecutable())
		return false;

	// Go go go!
	QProcess::start(fi.filePath());
	return true;
}


// Service slots.
void qtractorPluginFactoryProxy::stdout_slot (void)
{
	qtractorPluginFactory *pPluginFactory
		= static_cast<qtractorPluginFactory *> (QObject::parent());
	if (pPluginFactory == NULL)
		return;

	const QString sData(QProcess::readAllStandardOutput());
	addTypes(sData.split('\n'));
}


void qtractorPluginFactoryProxy::stderr_slot (void)
{
	QTextStream(stderr) << QProcess::readAllStandardError();
}


void qtractorPluginFactoryProxy::exit_slot (
	int exitCode, QProcess::ExitStatus exitStatus )
{
	if (m_iExitStatus < 0)
		m_iExitStatus = 0;

	if (exitCode || exitStatus != QProcess::NormalExit)
		++m_iExitStatus;
}


// Service methods.
bool qtractorPluginFactoryProxy::addTypes (
	qtractorPluginType::Hint typeHint, const QString& sFilename )
{
	// See if it's already cached in...
	if (!m_list.isEmpty()) {
		const QStringList& list = m_list.value(sFilename);
		if (list.isEmpty())
			return false;
		else
			return addTypes(list);
	}

	// Not cached, yet...
	++m_iFileCount;

	const QString& sHint = qtractorPluginType::textFromHint(typeHint);
	const QString& sLine = sHint + ':' + sFilename + '\n';
	const QByteArray& data = sLine.toUtf8();
	const bool bResult = (QProcess::write(data) == data.size());

	// Check for hideous scan crashes...
	if (!QProcess::waitForReadyRead(3000)) {
		if (m_iExitStatus > 0) {
			QProcess::waitForFinished(200);
			start(); // Restart the crashed scan...
			QProcess::waitForStarted(200);
		}
	}

	return bResult;
}


bool qtractorPluginFactoryProxy::addTypes ( const QStringList& list )
{
	qtractorPluginFactory *pPluginFactory
		= static_cast<qtractorPluginFactory *> (QObject::parent());
	if (pPluginFactory == NULL)
		return false;

	QStringListIterator iter(list);
	while (iter.hasNext()) {
		const QString& sText = iter.next().simplified();
		if (sText.isEmpty())
			continue;
		qtractorPluginType *pType = qtractorDummyPluginType::createType(sText);
		if (pType) {
			// Brand new type, add to inventory...
			pPluginFactory->addType(pType);
			// Cache in...
			if (m_file.isOpen())
				QTextStream(&m_file) << sText << "\n";
			// Done.
		} else {
			// Possibly some mistake occurred...
			QTextStream(stderr) << sText + '\n';
		}
	}

	return true;
}


// Absolute cache file path.
QString qtractorPluginFactoryProxy::cacheFilePath (void)
{
	const QString& sCacheDir
#if QT_VERSION < 0x050000
		= QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#else
		= QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#endif
	return QFileInfo(sCacheDir, "qtractor_vst_scan.cache").absoluteFilePath();
}


//----------------------------------------------------------------------------
// qtractorDummyPluginType -- Dummy plugin type instance.
//

// Constructor.
qtractorDummyPluginType::qtractorDummyPluginType (
	const QString& sText, unsigned long iIndex, Hint typeHint )
	: qtractorPluginType(NULL, iIndex, typeHint)
{
	const QStringList& props = sText.split('|');

	m_sName  = props.at(1);
	m_sLabel = m_sName.simplified().replace(QRegExp("[\\s|\\.|\\-]+"), "_");

	const QStringList& audios = props.at(2).split(':');
	m_iAudioIns  = audios.at(0).toUShort();
	m_iAudioOuts = audios.at(1).toUShort();

	const QStringList& midis = props.at(3).split(':');
	m_iMidiIns  = midis.at(0).toUShort();
	m_iMidiOuts = midis.at(1).toUShort();

	const QStringList& controls = props.at(4).split(':');
	m_iControlIns  = controls.at(0).toUShort();
	m_iControlOuts = controls.at(1).toUShort();

	const QStringList& flags = props.at(5).split(',');
	m_bEditor = flags.contains("GUI");
	m_bConfigure = flags.contains("EXT");
	m_bRealtime = flags.contains("RT");

	m_sFilename = props.at(6);

	bool bOk = false;
	QString sUniqueID = props.at(8);
	m_iUniqueID = qHash(sUniqueID.remove("0x").toULong(&bOk, 16));
}


// Must be overriden methods.
bool qtractorDummyPluginType::open (void)
{
	return true;
}


void qtractorDummyPluginType::close (void)
{
}


// Factory method (static)
qtractorDummyPluginType *qtractorDummyPluginType::createType (
	const QString& sText )
{
	// Sanity check...
	const QStringList& props = sText.split('|');
	if (props.count() < 7)
		return NULL;

	const Hint typeHint = qtractorPluginType::hintFromText(props.at(0));
	const unsigned long iIndex = props.at(7).toULong();

	// FIXME: Yep, most probably it's a dummy VST plugin effect...
	if (typeHint != Vst)
		return NULL;

	return new qtractorDummyPluginType(sText, iIndex, typeHint);
}


// end of qtractorPluginFactory.cpp
