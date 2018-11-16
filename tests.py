#! /usr/bin/python3

import pexpect
import unittest
import re
import os

keytab = {
	"<home>": "\033[1~",
	"<end>": "\033[4~",
	"<ins>": "\033[2~",
	"<del>": "\033[3~",
	"<pgup>": "\033[5~",
	"<pgdown>": "\033[6~",
	"<backspace>": "",
	"<tab>": "\t",
	"<cr>": "\r",
	"<lf>": "\n",
	"<left>": "\033OD",
	"<right>": "\033OC",
	"<up>": "\033OA",
	"<down>": "\033OB",
	"<c-left>": "\033[1;5D",
	"<c-right>": "\033[1;5C",
	"<c-up>": "\033[1;5A",
	"<c-down>": "\033[1;5B",
	"<c-c>": "",
	"<c-d>": "",
	"<c-a>": "",
	"<c-e>": "",
	"<c-w>": "",
	"<c-p>": "",
	"<c-n>": "",
	"<m-f>": "\033f",
	"<m-b>": "\033b",
	"<m-p>": "\033p",
	"<m-n>": "\033n",
	"<m-backspace>": "\033\177",
	"<f1>": "\033OP",
	"<f2>": "\033OQ"
}

termseq = {
	"\x1b[0m": "<rst>",
	"\x1b[J": "<ceos>",
	"\x1b[0;1;30m": "<gray>",
	"\x1b[0;22;31m": "<red>",
	"\x1b[0;22;32m": "<green>",
}

def sym_to_raw( str_ ):
	for sym, seq in keytab.items():
		str_ = str_.replace( sym, seq )
	return str_

colRe = re.compile( "\\x1b\\[(\\d+)G" )
upRe = re.compile( "\\x1b\\[(\\d+)A" )

def seq_to_sym( str_ ):
	for seq, sym in termseq.items():
		str_ = str_.replace( seq, sym )
	str_ = colRe.sub( "<c\\1>", str_ )
	str_ = upRe.sub( "<u\\1>", str_ )
	return str_

class ReplxxTests( unittest.TestCase ):
	_prompt_ = "\033\\[1;32mreplxx\033\\[0m> "
	@classmethod
	def setUpClass( cls ):
		os.environ["TERM"] = "xterm"
	def setUp( self_ ):
		self_._replxx = pexpect.spawn( "./build/example-cxx-api", maxread = 1, encoding = "utf-8", dimensions = ( 25, 80 ) )
		self_._replxx.expect( ReplxxTests._prompt_ )
		self_.maxDiff = None
	def check_scenario( self_, seq_, expected_ ):
		self_._replxx.send( sym_to_raw( seq_ ) )
		self_._replxx.expect( ReplxxTests._prompt_ + "\r\nExiting Replxx\r\n" )
		self_.assertSequenceEqual( seq_to_sym( self_._replxx.before ), expected_ )
	def test_home_key( self_ ):
		self_.check_scenario(
			"abc<home>z<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c9><c9><ceos>zabc<rst><c10><c9><ceos>zabc<rst><c13>\r\n"
			"zabc\r\n"
		)
	def test_end_key( self_ ):
		self_.check_scenario(
			"abc<home>z<end>q<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c9><c9><ceos>zabc<rst><c10><c9><ceos>zabc<rst><gray><rst><c13><c9><ceos>zabcq<rst><gray><rst><c14><c9><ceos>zabcq<rst><c14>\r\n"
			"zabcq\r\n"
		)
	def test_hint_show( self_ ):
		self_.check_scenario(
			"co\r<c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c11><c9><ceos>co<rst><c11>\r\n"
			"co\r\n"
		)
	def test_hint_scroll_down( self_ ):
		self_.check_scenario(
			"co<c-down><c-down><tab><cr><c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        "
			"<gray>color_green<rst><u3><c11><c9><ceos>co<rst><gray>lor_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst>\r\n"
			"        "
			"<gray>color_brown<rst><u3><c11><c9><ceos>co<rst><gray>lor_red<rst>\r\n"
			"        <gray>color_green<rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        "
			"<gray>color_blue<rst><u3><c11><c9><ceos><red>color_red<rst><green><rst><c18><c9><ceos><red>color_red<rst><c18>\r\n"
			"color_red\r\n"
		)

if __name__ == '__main__':
	unittest.main()

