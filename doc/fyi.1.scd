fyi(1)

# NAME
fyi - send desktop notifications

# SYNOPSIS
*fyi* [_OPTION_]... _TITLE_ [_MESSAGE_]++
*fyi --close*=_ID_++
*fyi --server-info*++
*fyi --server-capabilities*

# DESCRIPTION

*fyi* is a command line utility to send desktop notifications to the
user via a notification daemon implementing XDG desktop notifications.

It is similar to the well-known *notify-send*(1) utility. Indeed, most
of the options are identical.

When used without any options, *fyi* sends the notification and
immediately exits, without printing anything on stdout.

Use *--print-id* to have *fyi* print the daemon assigned notification
ID on stdout, in the format:

	*id=*_ID_

To see why the notification was closed, use *--print-reason*. The
reason is printed on stdout, in the format:

	*reason=*_REASON_

*fyi* can also block until the notification has been closed, using the
*--wait* option. Sending SIGINT to the *fyi* process will force-close
the notification.

*fyi* will also block if the notification has any actions. When an
action is triggered, *fyi* prints the name of the action on stdout, in
the format:

	*action=*_NAME_

Some notification daemons can send an "activation token". This is
typically done when the user clicks the notification, or triggers the
default action. The token can be used to focus (activate) a window. To
see the token, use *--print-token*. The token is printed on stdout, in
the format:

	*xdgtoken=*_TOKEN_

# OPTIONS

*-a*,*--app-name*=_NAME_
	Application name. Notification daemons will either display it as
	text, or use it to select an icon to show (unless *--icon* is
	used). Default: _fyi_

*-i*,*--icon*=_ICON_
	Icon to display, either as a symbolic icon name (e.g. _firefox_)
	or a filename. Default: _none_

*-u*,*--urgency*=*low|normal|critical*
	Notification urgency. Shortcut for
	*--hint=byte:urgency:0|1|2*. Default: _normal_.

*-c*,*--category*=_CATEGORY_
	Notification category. Shortcut for
	*--hint=string:category:CATEGORY*. Default: _none_.

*-A*,*--action*=_NAME_:_LABEL_
	Defines an action to display (e.g. as a button, or in a list,
	depending on notification daemon). _LABEL_ is what the
	notification daemon will display for the user. When the user
	triggers an action, *fyi* will print the corresponding _NAME_, in
	the format:
	
		*action=*_NAME_
	
	This option can be specified multiple times, to define multiple
	actions.
	
	Using this option implies *--wait*.

*-H*,*--hint*=_TYPE_:_NAME_:_VALUE_
	Defines a custom hint. How these are interpreted depends on the
	notification daemon. One common use case is to display a progress
	bar of some kind; most notification daemon recognizes
	*int:value:<percent>*.
	
	Another common hint is
	*string:x-canonical-private-synchronous:<name>*. Many notification
	daemons will replace any existing notification with the same
	_name_. This is similar to *--replaces*, except you do not need a
	notification ID.
	
	*fyi* recognizes the following types:
	
	- boolean
	- byte
	- int
	- double
	- string

*-r*,*--replaces*=_ID_
	If there is an existing notification with the specified ID,
	replace it. Otherwise, create a new notification.

*-t*,*--expire-time*=_TIME_
	The notification will be closed automatically after _TIME_
	milliseconds.

*--transient*
	By-pass the server's persistence capability, if any. Shortcut for
	*--hint=boolean:transient=true*.

*-C*,*--close*=_ID_
	If there is an existing notification with the specified ID, close
	it.

*-p*,*--print-id*
	Print the daemon assigned notification ID, in the format
	*id=*_ID_.

*-R*,*--print-reason*
	Print the reason the notification was closed, in the format:
	
	- *reason=expired*
	- *reason=dismissed*
	- *reason=force-closed*
	- *reason=unknown*
	
	Using this option implies *--wait*.

*-T*,*--print-token*
	Print the activation token, if any, in the format:
	
		*xdgtoken=*_TOKEN_
	
	Some notification daemons send an activation token when the
	notification is dismissed; either when the notification is
	clicked, or the default action is invoked. Others will send it
	regardless of which action was invoked. Some will only send it
	when an action is invoked, while others will send it when the
	notification is dismissed, regardless of how.
	
	The token can be used to focus (activate, raise) a window. It is
	not directly useable by *fyi*, but programs using *fyi* as a
	helper to display notifications can use it.
	
	Using this option implies *--wait*.

*-w*,*--wait*
	Wait for the notification to be closed before exiting. If the user
	triggered an action, the name of the action will be printed (see
	*-A*,*--action*).
	
	This option is implied when the any of the following options are
	used:
	
	- *-A*,*--action*
	- *-R*,*--print-reason*
	- *-T*,*--print-token*

*--image-data*=_FILE_
	Sets the *image-data* hint in the notification, with the raw pixel
	data from _FILE_. The data is assumed to be raw RGBA data. You
	must also provide the image size, see *--image-size*.

*--image-size=WIDTHxHEIGHT*
	The dimensions of the image loaded by --image-data.

*--server-info*
	Display notification daemon name and version.

*--server-capabilities*
	Display notification daemon capabilities.

*-v*,*--version*
	Show the version number and quit.

# SEE ALSO

- *notify-send*(1)
- *gdbus*(1)
