/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_DOWNLOAD_MANAGER_H
#define DCPLUSPLUS_DCPP_DOWNLOAD_MANAGER_H

#include "forward.h"

#include "DownloadManagerListener.h"
#include "UserConnectionListener.h"
#include "TimerManager.h"
#include "Singleton.h"
#include "Speaker.h"

#include "Bundle.h"
#include "CriticalSection.h"
#include "MerkleTree.h"

namespace dcpp {

/**
 * Singleton. Use its listener interface to update the download list
 * in the user interface.
 */
class DownloadManager : public Speaker<DownloadManagerListener>, 
	private UserConnectionListener, private TimerManagerListener, 
	public Singleton<DownloadManager>
{
public:

	/** @internal */
	void addConnection(UserConnection* conn);
	bool checkIdle(const UserPtr& user, bool smallSlot, bool reportOnly = false);

	void sendSizeUpdate(BundlePtr& aBundle) const noexcept;
	BundlePtr findRunningBundle(QueueToken bundleToken) const noexcept;

	/** @internal */
	void abortDownload(const string& aTarget, const UserPtr& aUser = nullptr);
	void disconnectBundle(BundlePtr& aBundle, const UserPtr& aUser = nullptr);

	/** @return Running average download speed in Bytes/s */
	int64_t getRunningAverage() const;

	/** @return Number of downloads. */ 
	size_t getDownloadCount() const {
		RLock l(cs);
		return downloads.size();
	}
	size_t getDownloadCount(const BundlePtr& aBundle) const {
		RLock l(cs);
		return aBundle->getDownloads().size();
	}

	// This will ignore bundles with no downloads and 
	// bundles using highest priority
	void getRunningBundles(QueueTokenSet& bundles_) const;

	SharedMutex& getCS() { return cs; }
	const DownloadList& getDownloads() const {
		return downloads;
	}

	IGETSET(int64_t, lastUpSpeed, LastUpSpeed, 0);
	IGETSET(int64_t, lastDownSpeed, LastDownSpeed, 0);
private:
	
	mutable SharedMutex cs;
	DownloadList downloads;

	// The list of bundles being download. Note that all of them may not be running
	// as the bundle is removed from here only after the connection has been 
	// switched to use another bundle (or no other downloads were found)
	Bundle::TokenBundleMap bundles;
	UserConnectionList idlers;

	void removeRunningUser(UserConnection* aSource, bool sendRemoved=false) noexcept;
	void removeConnection(UserConnectionPtr aConn);
	void removeDownload(Download* aDown);
	void fileNotAvailable(UserConnection* aSource, bool noAccess);
	void noSlots(UserConnection* aSource, string param = Util::emptyString);

	void failDownload(UserConnection* aSource, const string& reason, bool rotateQueue);

	friend class Singleton<DownloadManager>;

	DownloadManager();
	~DownloadManager();

	//typedef unordered_set<CID> CIDList;
	void checkDownloads(UserConnection* aConn);
	void startData(UserConnection* aSource, int64_t start, int64_t newSize, bool z);
	void startBundle(UserConnection* aSource, BundlePtr aBundle);

	void revive(UserConnection* uc);
	void endData(UserConnection* aSource);

	void onFailed(UserConnection* aSource, const string& aError);

	// UserConnectionListener
	void on(Data, UserConnection*, const uint8_t*, size_t) noexcept;
	void on(Failed, UserConnection* aSource, const string& aError) noexcept { onFailed(aSource, aError); }
	void on(ProtocolError, UserConnection* aSource, const string& aError) noexcept { onFailed(aSource, aError); }
	void on(MaxedOut, UserConnection*, const string& param) noexcept;
	void on(FileNotAvailable, UserConnection*) noexcept;
		
	void on(AdcCommand::SND, UserConnection*, const AdcCommand&) noexcept;
	void on(AdcCommand::STA, UserConnection*, const AdcCommand&) noexcept;

	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t aTick) noexcept;

	typedef pair< string, int64_t > StringIntPair;

	// Statistics
	uint64_t lastUpdate = 0;
	int64_t lastUpBytes = 0;
	int64_t lastDownBytes = 0;
};

} // namespace dcpp

#endif // !defined(DOWNLOAD_MANAGER_H)