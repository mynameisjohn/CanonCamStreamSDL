#pragma once

#include <EDSDK.h>

#include "CameraApp.h"

class CameraEventListener
{
public:
	static EdsError EDSCALLBACK  handleObjectEvent (
		EdsUInt32			inEvent,
		EdsBaseRef			inRef,
		EdsVoid *			inContext
		);

	static EdsError EDSCALLBACK  handlePropertyEvent (
		EdsUInt32			inEvent,
		EdsUInt32			inPropertyID,
		EdsUInt32			inParam,
		EdsVoid *			inContext
		);

	static EdsError EDSCALLBACK  handleStateEvent (
		EdsUInt32			inEvent,
		EdsUInt32			inParam,
		EdsVoid *			inContext
		);

};