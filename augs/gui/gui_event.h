#pragma once

enum class gui_event {
	unknown,
	keydown,
	keyup,
	character,
	unichar,
	wheel,

	lpressed,
	rpressed,
	ldown,
	mdown,
	rdown,
	lclick,
	rclick,
	ldoubleclick,
	ltripleclick,
	mdoubleclick,
	rdoubleclick,
	lup,
	mup,
	rup,
	loutup,
	routup,
	hover,
	hoverlost,
	hout,
	lstarteddrag,
	ldrag,
	loutdrag,
	lfinisheddrag,

	focus,
	blur
};