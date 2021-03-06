/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/timer.h"

namespace Main {
class Session;
} // namespace Main

namespace Platform {
namespace Notifications {
class Manager;
} // namespace Notifications
} // namespace Platform

namespace Media {
namespace Audio {
class Track;
} // namespace Audio
} // namespace Media

namespace Window {
namespace Notifications {

enum class ChangeType {
	SoundEnabled,
	IncludeMuted,
	CountMessages,
	DesktopEnabled,
	ViewParams,
	MaxCount,
	Corner,
	DemoIsShown,
};

} // namespace Notifications
} // namespace Window

namespace base {

template <>
struct custom_is_fast_copy_type<Window::Notifications::ChangeType> : public std::true_type {
};

} // namespace base

namespace Window {
namespace Notifications {

class Manager;

class System final : private base::Subscriber {
public:
	explicit System(not_null<Main::Session*> session);

	void createManager();

	void checkDelayed();
	void schedule(not_null<HistoryItem*> item);
	void clearFromHistory(not_null<History*> history);
	void clearIncomingFromHistory(not_null<History*> history);
	void clearFromItem(not_null<HistoryItem*> item);
	void clearAll();
	void clearAllFast();
	void updateAll();

	base::Observable<ChangeType> &settingsChanged() {
		return _settingsChanged;
	}

	Main::Session &session() const {
		return *_session;
	}

	~System();

private:
	struct SkipState {
		enum Value {
			Unknown,
			Skip,
			DontSkip
		};
		Value value = Value::Unknown;
		bool silent = false;
	};

	SkipState skipNotification(not_null<HistoryItem*> item) const;

	void showNext();
	void showGrouped();
	void ensureSoundCreated();

	not_null<Main::Session*> _session;

	QMap<History*, QMap<MsgId, crl::time>> _whenMaps;

	struct Waiter {
		Waiter(MsgId msg, crl::time when, PeerData *notifyBy)
		: msg(msg)
		, when(when)
		, notifyBy(notifyBy) {
		}
		MsgId msg;
		crl::time when;
		PeerData *notifyBy;
	};
	using Waiters = QMap<History*, Waiter>;
	Waiters _waiters;
	Waiters _settingWaiters;
	base::Timer _waitTimer;
	base::Timer _waitForAllGroupedTimer;

	QMap<History*, QMap<crl::time, PeerData*>> _whenAlerts;

	std::unique_ptr<Manager> _manager;

	base::Observable<ChangeType> _settingsChanged;

	std::unique_ptr<Media::Audio::Track> _soundTrack;

	int _lastForwardedCount = 0;
	FullMsgId _lastHistoryItemId;

};

class Manager {
public:
	explicit Manager(not_null<System*> system) : _system(system) {
	}

	void showNotification(
			not_null<HistoryItem*> item,
			int forwardedCount) {
		doShowNotification(item, forwardedCount);
	}
	void updateAll() {
		doUpdateAll();
	}
	void clearAll() {
		doClearAll();
	}
	void clearAllFast() {
		doClearAllFast();
	}
	void clearFromItem(not_null<HistoryItem*> item) {
		doClearFromItem(item);
	}
	void clearFromHistory(not_null<History*> history) {
		doClearFromHistory(history);
	}

	void notificationActivated(PeerId peerId, MsgId msgId);
	void notificationReplied(
		PeerId peerId,
		MsgId msgId,
		const TextWithTags &reply);

	struct DisplayOptions {
		bool hideNameAndPhoto;
		bool hideMessageText;
		bool hideReplyButton;
	};
	static DisplayOptions getNotificationOptions(HistoryItem *item);

	virtual ~Manager() = default;

protected:
	not_null<System*> system() const {
		return _system;
	}

	virtual void doUpdateAll() = 0;
	virtual void doShowNotification(
		not_null<HistoryItem*> item,
		int forwardedCount) = 0;
	virtual void doClearAll() = 0;
	virtual void doClearAllFast() = 0;
	virtual void doClearFromItem(not_null<HistoryItem*> item) = 0;
	virtual void doClearFromHistory(not_null<History*> history) = 0;
	virtual void onBeforeNotificationActivated(PeerId peerId, MsgId msgId) {
	}
	virtual void onAfterNotificationActivated(PeerId peerId, MsgId msgId) {
	}

private:
	void openNotificationMessage(
		not_null<History*> history,
		MsgId messageId);

	const not_null<System*> _system;

};

class NativeManager : public Manager {
protected:
	using Manager::Manager;

	void doUpdateAll() override {
		doClearAllFast();
	}
	void doClearAll() override {
		doClearAllFast();
	}
	void doClearFromItem(not_null<HistoryItem*> item) override {
	}
	void doShowNotification(
		not_null<HistoryItem*> item,
		int forwardedCount) override;

	virtual void doShowNativeNotification(
		not_null<PeerData*> peer,
		MsgId msgId,
		const QString &title,
		const QString &subtitle,
		const QString &msg,
		bool hideNameAndPhoto,
		bool hideReplyButton) = 0;

};

QString WrapFromScheduled(const QString &text);

} // namespace Notifications
} // namespace Window
