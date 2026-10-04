#!/usr/bin/env python3
# Generates the Tafat-style toolbar and bottom-bar icons (artwork/feature-*, panel-*, button-*).
# usage: tools/generate-toolbar-icons.py artwork && tools/render-artwork.sh
import os, sys

OUT = sys.argv[1]
TEAL, ORANGE, YELLOW, PAPER, CREAM2, BROWN2, CREAM = '#13705f', '#f26b2d', '#f6bf3f', '#fffdf9', '#f4ebdd', '#4b3427', '#faf4ea'
TEAL2 = '#1f9d86'
SW = 7

def svg(name, comment, body):
	with open(os.path.join(OUT, name + '.svg'), 'w') as f:
		f.write('<?xml version="1.0" encoding="UTF-8"?>\n<!-- %s -->\n'
		        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="128" height="128">\n%s</svg>\n' % (comment, body))

def screen(x=12, y=14, w=104, h=72, stand=True, fill=PAPER, inner=CREAM2, stroke=TEAL):
	s = ''
	if stand:
		cx = x + w / 2
		s += '  <path d="M%g %g v14 M%g %g h%g" stroke="%s" stroke-width="%d" stroke-linecap="round"/>\n' % (cx, y + h, cx - 20, y + h + 16, 40, stroke, SW)
	s += '  <rect x="%g" y="%g" width="%g" height="%g" rx="10" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (x, y, w, h, fill, stroke, SW)
	s += '  <rect x="%g" y="%g" width="%g" height="%g" rx="4" fill="%s"/>\n' % (x + 9, y + 9, w - 18, h - 18, inner)
	return s

def person(cx, cy, r, color, body_w=None):
	bw = body_w or r * 3.2
	return ('  <circle cx="%g" cy="%g" r="%g" fill="%s"/>\n' % (cx, cy, r, color) +
	        '  <path d="M%g %g a%g %g 0 0 1 %g 0 z" fill="%s"/>\n' % (cx - bw / 2, cy + r * 3.4, bw / 2, bw / 2.2, bw, color))

def power_symbol(cx, cy, r, color, width=SW):
	return ('  <path d="M%g %g A%g %g 0 1 0 %g %g" fill="none" stroke="%s" stroke-width="%g" stroke-linecap="round"/>\n'
	        % (cx - r * 0.62, cy - r * 0.78, r, r, cx + r * 0.62, cy - r * 0.78, color, width) +
	        '  <path d="M%g %g v%g" stroke="%s" stroke-width="%g" stroke-linecap="round"/>\n' % (cx, cy - r * 1.15, r * 1.05, color, width))

# ---------------------------------------------------------------- toolbar
svg('feature-monitoring', 'Monitoring: the screens of the room',
	''.join(screen(x, y, 50, 40, stand=False, inner=(YELLOW if (x, y) == (68, 16) else CREAM2)) for x in (10, 68) for y in (16, 70)))

svg('feature-demo', 'Demo: the teacher screen shown to everybody',
	screen() +
	'  <path d="M52 34 l26 16 l-26 16 z" fill="%s" stroke="%s" stroke-width="5" stroke-linejoin="round"/>\n' % (ORANGE, ORANGE) +
	'  <g fill="none" stroke="%s" stroke-width="6" stroke-linecap="round"><path d="M88 32 a22 22 0 0 1 0 36"/><path d="M98 24 a34 34 0 0 1 0 52"/></g>\n' % YELLOW)

svg('feature-demo-fullscreen', 'Demo in full screen',
	screen() +
	'  <g fill="none" stroke="%s" stroke-width="6" stroke-linecap="round" stroke-linejoin="round">'
	'<path d="M30 42 v-10 h12"/><path d="M98 42 v-10 h-12"/><path d="M30 58 v10 h12"/><path d="M98 58 v10 h-12"/></g>\n' % ORANGE)

svg('feature-demo-window', 'Demo in a window',
	screen() +
	'  <rect x="36" y="32" width="56" height="36" rx="5" fill="%s" stroke="%s" stroke-width="5"/>\n' % (PAPER, ORANGE) +
	'  <rect x="36" y="32" width="56" height="10" rx="4" fill="%s"/>\n' % ORANGE)

svg('feature-lock', 'Lock: the student screens are locked',
	screen(inner=BROWN2) +
	'  <path d="M52 48 v-8 a12 12 0 0 1 24 0 v8" fill="none" stroke="%s" stroke-width="7" stroke-linecap="round"/>\n' % YELLOW +
	'  <rect x="44" y="46" width="40" height="28" rx="6" fill="%s"/>\n' % ORANGE +
	'  <circle cx="64" cy="58" r="4" fill="%s"/><rect x="62" y="58" width="4" height="9" rx="2" fill="%s"/>\n' % (PAPER, PAPER))

svg('feature-remote-view', 'Remote view: watch one screen',
	screen() +
	'  <circle cx="80" cy="66" r="18" fill="%s" stroke="%s" stroke-width="7"/>\n' % (PAPER, ORANGE) +
	'  <path d="M93 79 l20 20" stroke="%s" stroke-width="10" stroke-linecap="round"/>\n' % ORANGE)

svg('feature-remote-control', 'Remote control: take over one computer',
	screen() +
	'  <path d="M56 30 v44 l11 -10 l8 18 l9 -4 l-8 -17 h15 z" fill="%s" stroke="%s" stroke-width="5" stroke-linejoin="round"/>\n' % (ORANGE, PAPER))

svg('feature-power-on', 'Power on',
	'  <circle cx="64" cy="64" r="50" fill="%s"/>\n' % TEAL2 + power_symbol(64, 68, 24, PAPER, 9))

svg('feature-power-down', 'Power down',
	'  <circle cx="64" cy="64" r="50" fill="%s"/>\n' % ORANGE + power_symbol(64, 68, 24, PAPER, 9))

svg('feature-reboot', 'Reboot',
	'  <circle cx="64" cy="64" r="50" fill="%s"/>\n' % YELLOW +
	'  <path d="M88 60 A25 25 0 1 1 78 42" fill="none" stroke="%s" stroke-width="9" stroke-linecap="round"/>\n' % BROWN2 +
	'  <path d="M70 30 l18 6 l-6 18 z" fill="%s" stroke="%s" stroke-width="4" stroke-linejoin="round"/>\n' % (BROWN2, BROWN2))

def door_arrow(out):
	door = '  <rect x="62" y="20" width="48" height="88" rx="9" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (PAPER, TEAL, SW)
	door += '  <circle cx="74" cy="66" r="5" fill="%s"/>\n' % YELLOW
	if out:
		arrow = '  <path d="M58 64 h-40 m12 -14 l-14 14 l14 14" fill="none" stroke="%s" stroke-width="9" stroke-linecap="round" stroke-linejoin="round"/>\n' % ORANGE
	else:
		arrow = '  <path d="M18 64 h56 m-14 -14 l14 14 l-14 14" fill="none" stroke="%s" stroke-width="9" stroke-linecap="round" stroke-linejoin="round"/>\n' % TEAL2
	return door + arrow

svg('feature-log-in', 'Log in a user', door_arrow(False))
svg('feature-log-off', 'Log off the user', door_arrow(True))

svg('feature-text-message', 'Text message to the students',
	'  <path d="M20 16 h88 a10 10 0 0 1 10 10 v52 a10 10 0 0 1 -10 10 h-50 l-22 20 v-20 h-16 a10 10 0 0 1 -10 -10 v-52 a10 10 0 0 1 10 -10 z" fill="%s" stroke="%s" stroke-width="%d" stroke-linejoin="round"/>\n' % (PAPER, TEAL, SW) +
	'  <circle cx="64" cy="34" r="6" fill="%s"/><rect x="58" y="46" width="12" height="30" rx="6" fill="%s"/>\n' % (ORANGE, ORANGE))

svg('feature-start-app', 'Start an application on the student computers',
	'  <rect x="12" y="18" width="104" height="88" rx="12" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (PAPER, TEAL, SW) +
	'  <path d="M12 40 h104" stroke="%s" stroke-width="%d"/>\n' % (TEAL, SW) +
	'  <g fill="%s"><circle cx="28" cy="29" r="4"/><circle cx="42" cy="29" r="4"/></g>\n' % ORANGE +
	'  <path d="M54 54 l28 18 l-28 18 z" fill="%s" stroke="%s" stroke-width="5" stroke-linejoin="round"/>\n' % (ORANGE, ORANGE))

svg('feature-open-website', 'Open a website on the student computers',
	'  <circle cx="60" cy="60" r="46" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (PAPER, TEAL, SW) +
	'  <g fill="none" stroke="%s" stroke-width="5"><ellipse cx="60" cy="60" rx="20" ry="46"/><path d="M16 46 h88 M16 74 h88 M60 14 v92"/></g>\n' % TEAL +
	'  <path d="M78 70 v42 l10 -9 l7 16 l8 -4 l-7 -15 h13 z" fill="%s" stroke="%s" stroke-width="5" stroke-linejoin="round"/>\n' % (ORANGE, PAPER))

def folder(fill=PAPER):
	return '  <path d="M14 40 v-14 a8 8 0 0 1 8 -8 h26 l10 10 h48 a8 8 0 0 1 8 8 v60 a8 8 0 0 1 -8 8 h-84 a8 8 0 0 1 -8 -8 z" fill="%s" stroke="%s" stroke-width="%d" stroke-linejoin="round"/>\n' % (fill, TEAL, SW)

svg('feature-distribute', 'Distribute files to the students',
	folder() +
	'  <path d="M64 92 v-40 m-16 16 l16 -16 l16 16" fill="none" stroke="%s" stroke-width="9" stroke-linecap="round" stroke-linejoin="round"/>\n' % ORANGE)

svg('feature-collect', 'Collect files from the students',
	folder() +
	'  <path d="M64 50 v40 m-16 -16 l16 16 l16 -16" fill="none" stroke="%s" stroke-width="9" stroke-linecap="round" stroke-linejoin="round"/>\n' % TEAL2)

camera = ('  <path d="M14 42 a8 8 0 0 1 8 -8 h18 l8 -12 h32 l8 12 h18 a8 8 0 0 1 8 8 v54 a8 8 0 0 1 -8 8 h-84 a8 8 0 0 1 -8 -8 z" fill="%s" stroke="%s" stroke-width="%d" stroke-linejoin="round"/>\n' % (PAPER, TEAL, SW) +
          '  <circle cx="64" cy="66" r="22" fill="%s"/><circle cx="64" cy="66" r="10" fill="%s"/>\n' % (ORANGE, YELLOW) +
          '  <circle cx="98" cy="48" r="5" fill="%s"/>\n' % TEAL2)
svg('feature-screenshot', 'Screenshot', camera)

# ---------------------------------------------------------------- bottom bar
svg('panel-computers', 'Locations & computers panel',
	screen(8, 26, 70, 50) + screen(50, 12, 70, 50, stand=False, inner=YELLOW))
svg('panel-slideshow', 'Slideshow panel',
	screen() +
	'  <g fill="none" stroke="%s" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"><path d="M44 36 l-12 14 l12 14"/><path d="M84 36 l12 14 l-12 14"/></g>\n' % ORANGE +
	'  <rect x="52" y="38" width="24" height="24" rx="4" fill="%s"/>\n' % YELLOW)
svg('panel-spotlight', 'Spotlight panel',
	screen() +
	'  <circle cx="64" cy="50" r="20" fill="%s"/>\n' % YELLOW +
	'  <path d="M64 38 l4 8 l9 1 l-7 6 l2 9 l-8 -5 l-8 5 l2 -9 l-7 -6 l9 -1 z" fill="%s"/>\n' % ORANGE)

def small(name, comment, body_fn):
	# small buttons of the bottom bar: a light variant (teal) and a dark one (cream)
	svg(name, comment, body_fn(TEAL, ORANGE))
	svg(name + '-dark', comment + ' (dark theme)', body_fn(CREAM, ORANGE))

small('button-powered-on', 'Show only computers that are switched on',
	lambda c, a: power_symbol(64, 68, 34, c, 12))
small('button-align-grid', 'Align the computers in a grid',
	lambda c, a: ''.join('  <rect x="%d" y="%d" width="28" height="28" rx="6" fill="%s"/>\n' % (14 + 36 * i, 14 + 36 * j, a if (i, j) == (1, 1) else c)
	                      for i in range(3) for j in range(3)))
small('button-zoom-fit', 'Fit the computers to the window',
	lambda c, a: '  <rect x="38" y="38" width="52" height="52" rx="8" fill="%s"/>\n' % a +
	             '  <g fill="none" stroke="%s" stroke-width="11" stroke-linecap="round" stroke-linejoin="round">'
	             '<path d="M14 40 v-26 h26"/><path d="M114 40 v-26 h-26"/><path d="M14 88 v26 h26"/><path d="M114 88 v26 h-26"/></g>\n' % c)
small('button-exchange', 'Change the order of the computers',
	lambda c, a: '  <rect x="12" y="12" width="56" height="56" rx="10" fill="none" stroke="%s" stroke-width="10"/>\n' % c +
	             '  <rect x="60" y="60" width="56" height="56" rx="10" fill="%s"/>\n' % a +
	             '  <g fill="none" stroke="%s" stroke-width="9" stroke-linecap="round" stroke-linejoin="round"><path d="M86 18 h14 v24 m-10 -10 l10 10 l10 -10"/><path d="M42 110 h-14 v-24 m10 10 l-10 -10 l-10 10"/></g>\n' % c)
small('button-about', 'About',
	lambda c, a: '  <circle cx="64" cy="64" r="50" fill="none" stroke="%s" stroke-width="11"/>\n' % c +
	             '  <circle cx="64" cy="38" r="8" fill="%s"/><rect x="56" y="54" width="16" height="44" rx="8" fill="%s"/>\n' % (a, a))
small('button-user-group', 'Users and groups',
	lambda c, a: person(46, 36, 15, c, 56) + person(86, 44, 12, c, 44) +
	             '  <circle cx="98" cy="98" r="22" fill="%s"/><path d="M98 86 v24 M86 98 h24" stroke="#fffdf9" stroke-width="8" stroke-linecap="round"/>\n' % a)

def toolbar_toggle(c, a, up):
	# the teacher's toolbar (a bar of three buttons) above a chevron pointing where it goes
	chevron = 'M40 104 l24 -20 l24 20' if up else 'M40 84 l24 20 l24 -20'
	return ('  <rect x="10" y="14" width="108" height="44" rx="10" fill="none" stroke="%s" stroke-width="10"/>\n' % c +
	        ''.join('  <rect x="%d" y="28" width="16" height="16" rx="4" fill="%s"/>\n' % (x, c) for x in (28, 56, 84)) +
	        '  <path d="%s" fill="none" stroke="%s" stroke-width="12" stroke-linecap="round" stroke-linejoin="round"/>\n' % (chevron, a))
small('button-toolbar-hide', 'Hide the toolbar', lambda c, a: toolbar_toggle(c, a, True))
small('button-toolbar-show', 'Show the toolbar', lambda c, a: toolbar_toggle(c, a, False))

# ---------------------------------------------------------------- configurator pages
svg('page-general', 'Configurator page: general settings',
	''.join('  <path d="M18 %d h92" stroke="%s" stroke-width="%d" stroke-linecap="round"/>\n  <path d="M18 %d h92" stroke="%s" stroke-width="%d" stroke-linecap="round"/>\n'
	        % (y, PAPER, SW + 8, y, TEAL2, SW + 1) for y in (34, 64, 94)) +
	''.join('  <circle cx="%d" cy="%d" r="12" fill="%s" stroke="%s" stroke-width="6"/>\n' % (x, y, c, TEAL)
	        for x, y, c in ((44, 34, ORANGE), (84, 64, YELLOW), (56, 94, PAPER))))

def gear(cx, cy, r, color, hole):
	teeth = ''.join('<rect x="%g" y="%g" width="16" height="22" rx="4" transform="rotate(%d %g %g)"/>' % (cx - 8, cy - r - 12, a, cx, cy) for a in range(0, 360, 45))
	return ('  <g fill="%s">%s<circle cx="%g" cy="%g" r="%g"/></g>\n' % (color, teeth, cx, cy, r) +
	        '  <circle cx="%g" cy="%g" r="%g" fill="%s"/>\n' % (cx, cy, r * 0.42, hole))

svg('page-service', 'Configurator page: the Tafat service',
	gear(64, 64, 36, TEAL2, PAPER) + '  <circle cx="64" cy="64" r="8" fill="%s"/>\n' % ORANGE)

svg('page-master', 'Configurator page: the teacher program',
	screen() + ''.join('  <rect x="%d" y="%d" width="22" height="16" rx="3" fill="%s"/>\n' % (x, y, c)
	                   for x, y, c in ((30, 32, TEAL2), (53, 32, YELLOW), (76, 32, TEAL2), (30, 52, TEAL2), (53, 52, ORANGE), (76, 52, TEAL2))))

svg('page-access-control', 'Configurator page: access control',
	'  <path d="M64 12 l42 14 v30 c0 28 -18 48 -42 60 c-24 -12 -42 -32 -42 -60 v-30 z" fill="%s" stroke="%s" stroke-width="%d" stroke-linejoin="round"/>\n' % (PAPER, TEAL, SW) +
	'  <circle cx="64" cy="54" r="12" fill="%s"/><path d="M58 60 h12 l4 26 h-20 z" fill="%s"/>\n' % (ORANGE, ORANGE))

svg('page-keys', 'Configurator page: authentication keys',
	'  <circle cx="42" cy="64" r="26" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (YELLOW, TEAL, SW) +
	'  <circle cx="38" cy="64" r="8" fill="%s"/>\n' % PAPER +
	'  <path d="M68 64 h48 M100 64 v18 M86 64 v12" stroke="%s" stroke-width="10" stroke-linecap="round"/>\n' % TEAL)

svg('page-locations', 'Configurator page: locations and computers',
	'  <path d="M14 56 l50 -38 l50 38 v52 a6 6 0 0 1 -6 6 h-88 a6 6 0 0 1 -6 -6 z" fill="%s" stroke="%s" stroke-width="%d" stroke-linejoin="round"/>\n' % (PAPER, TEAL, SW) +
	''.join('  <rect x="%d" y="%d" width="26" height="18" rx="3" fill="%s"/><path d="M%d %d v6" stroke="%s" stroke-width="4"/>\n'
	        % (x, y, c, x + 13, y + 18, TEAL, ) for x, y, c in ((28, 62, TEAL2), (74, 62, YELLOW), (51, 86, ORANGE))))

svg('page-apps-websites', 'Configurator page: applications and websites',
	'  <rect x="10" y="20" width="84" height="70" rx="10" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (PAPER, TEAL, SW) +
	'  <path d="M10 40 h84" stroke="%s" stroke-width="%d"/>\n' % (TEAL, SW) +
	'  <path d="M36 52 l22 14 l-22 14 z" fill="%s"/>\n' % ORANGE +
	'  <circle cx="92" cy="86" r="28" fill="%s" stroke="%s" stroke-width="6"/>\n' % (PAPER, TEAL) +
	'  <g fill="none" stroke="%s" stroke-width="4"><ellipse cx="92" cy="86" rx="11" ry="28"/><path d="M64 86 h56"/></g>\n' % TEAL)

svg('page-file-transfer', 'Configurator page: file transfer',
	folder() +
	'  <g fill="none" stroke-width="8" stroke-linecap="round" stroke-linejoin="round">'
	'<path d="M48 90 v-34 m-12 12 l12 -12 l12 12" stroke="%s"/><path d="M80 56 v34 m-12 -12 l12 12 l12 -12" stroke="%s"/></g>\n' % (ORANGE, TEAL2))

svg('page-demo', 'Configurator page: demo',
	screen(8, 12, 72, 52, stand=False) + screen(46, 44, 72, 52, inner=YELLOW))

svg('page-ldap', 'Configurator page: LDAP / Active Directory',
	''.join('  <ellipse cx="64" cy="%d" rx="40" ry="12" fill="%s" stroke="%s" stroke-width="6"/>\n' % (y, PAPER, TEAL) for y in ()) +
	'  <path d="M24 28 v72 a40 12 0 0 0 80 0 v-72" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (PAPER, TEAL, SW) +
	'  <ellipse cx="64" cy="28" rx="40" ry="12" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (YELLOW, TEAL, SW) +
	'  <path d="M24 52 a40 12 0 0 0 80 0 M24 76 a40 12 0 0 0 80 0" fill="none" stroke="%s" stroke-width="5"/>\n' % TEAL +
	'  <circle cx="90" cy="66" r="5" fill="%s"/><circle cx="90" cy="90" r="5" fill="%s"/>\n' % (ORANGE, ORANGE))

svg('page-webapi', 'Configurator page: web API',
	'  <rect x="10" y="18" width="108" height="92" rx="12" fill="%s" stroke="%s" stroke-width="%d"/>\n' % (PAPER, TEAL, SW) +
	'  <g fill="none" stroke="%s" stroke-width="9" stroke-linecap="round" stroke-linejoin="round"><path d="M44 46 l-18 18 l18 18"/><path d="M84 46 l18 18 l-18 18"/></g>\n' % TEAL +
	'  <path d="M70 40 l-12 48" stroke="%s" stroke-width="9" stroke-linecap="round"/>\n' % ORANGE)

# access control rule states
svg('rule-allow', 'Access rule: allow', '  <circle cx="64" cy="64" r="50" fill="%s"/><path d="M40 66 l16 16 l32 -34" fill="none" stroke="%s" stroke-width="12" stroke-linecap="round" stroke-linejoin="round"/>\n' % (TEAL2, PAPER))
svg('rule-deny', 'Access rule: deny', '  <circle cx="64" cy="64" r="50" fill="%s"/><path d="M44 44 l40 40 M84 44 l-40 40" stroke="%s" stroke-width="12" stroke-linecap="round"/>\n' % (ORANGE, PAPER))
svg('rule-ask', 'Access rule: ask for permission', '  <circle cx="64" cy="64" r="50" fill="%s"/><path d="M48 50 a16 16 0 1 1 22 15 c-5 2 -6 5 -6 10" fill="none" stroke="%s" stroke-width="11" stroke-linecap="round"/><circle cx="64" cy="94" r="7" fill="%s"/>\n' % (YELLOW, BROWN2, BROWN2))
svg('rule-none', 'Access rule: no action', '  <circle cx="64" cy="64" r="50" fill="none" stroke="%s" stroke-width="10"/><path d="M42 64 h44" stroke="%s" stroke-width="12" stroke-linecap="round"/>\n' % (TEAL, TEAL))

# ---------------------------------------------------------------- Tafat features
svg('feature-rewards', 'Rewards: give a star to a student',
	'  <path d="M64 10 l15 32 l35 4 l-26 24 l7 35 l-31 -18 l-31 18 l7 -35 l-26 -24 l35 -4 z" fill="%s" stroke="%s" stroke-width="%d" stroke-linejoin="round"/>\n' % (YELLOW, ORANGE, SW) +
	'  <circle cx="64" cy="62" r="13" fill="%s"/>\n' % PAPER +
	'  <path d="M58 62 l5 5 l9 -10" fill="none" stroke="%s" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>\n' % TEAL)
