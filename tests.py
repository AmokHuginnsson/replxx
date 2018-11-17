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
	"<c-a>": "",
	"<c-b>": "",
	"<c-c>": "",
	"<c-d>": "",
	"<c-e>": "",
	"<c-f>": "",
	"<c-k>": "",
	"<c-l>": "",
	"<c-n>": "",
	"<c-p>": "",
	"<c-r>": "",
	"<c-s>": "",
	"<c-u>": "",
	"<c-v>": "",
	"<c-w>": "",
	"<c-y>": "",
	"<m-b>": "\033b",
	"<m-d>": "\033d",
	"<m-f>": "\033f",
	"<m-n>": "\033n",
	"<m-p>": "\033p",
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
	"\x1b[0;1;31m": "<brightred>",
	"\x1b[0;1;34m": "<brightblue>",
	"\x1b[0;1;35m": "<brightmagenta>",
	"\x1b[0;1;37m": "<white>",
	"\x1b[101;1;33m": "<err>"
}
colRe = re.compile( "\\x1b\\[(\\d+)G" )
upRe = re.compile( "\\x1b\\[(\\d+)A" )

def sym_to_raw( str_ ):
	for sym, seq in keytab.items():
		str_ = str_.replace( sym, seq )
	return str_

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
		with open( "replxx_history.txt", "wb" ) as f:
			f.write( "one\ntwo\nthree\n".encode() )
			f.close()
		self_._replxx = pexpect.spawn( "./build/example-cxx-api", maxread = 1, encoding = "utf-8", dimensions = ( 25, 80 ) )
		self_._replxx.expect( ReplxxTests._prompt_ )
		self_.maxDiff = None
	def check_scenario( self_, seq_, expected_ ):
		self_._replxx.send( sym_to_raw( seq_ ) )
		self_._replxx.expect( ReplxxTests._prompt_ + "\r\nExiting Replxx\r\n" )
		self_.assertSequenceEqual( seq_to_sym( self_._replxx.before ), expected_ )
	def test_ctrl_c( self_ ):
		self_.check_scenario(
			"abc<c-c><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c12>^C\r"
			"\r\n"
		)
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
	def test_left_key( self_ ):
		self_.check_scenario(
			"abc<left>x<left><left>y<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c11><c9><ceos>abxc<rst><c12><c9><ceos>abxc<rst><c11><c9><ceos>abxc<rst><c10><c9><ceos>aybxc<rst><c11><c9><ceos>aybxc<rst><c14>\r\n"
			"aybxc\r\n"
		)
	def test_right_key( self_ ):
		self_.check_scenario(
			"abc<home><right>x<right>y<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c9><c9><ceos>abc<rst><c10><c9><ceos>axbc<rst><c11><c9><ceos>axbc<rst><c12><c9><ceos>axbyc<rst><c13><c9><ceos>axbyc<rst><c14>\r\n"
			"axbyc\r\n"
		)
	def test_prev_word_key( self_ ):
		self_.check_scenario(
			"abc def ghi<c-left><c-left>x<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc "
			"<rst><gray><rst><c13><c9><ceos>abc d<rst><gray><rst><c14><c9><ceos>abc "
			"de<rst><gray><rst><c15><c9><ceos>abc def<rst><gray><rst><c16><c9><ceos>abc "
			"def <rst><gray><rst><c17><c9><ceos>abc def "
			"g<rst><gray><rst><c18><c9><ceos>abc def gh<rst><gray><rst><c19><c9><ceos>abc "
			"def ghi<rst><gray><rst><c20><c9><ceos>abc def ghi<rst><c17><c9><ceos>abc def "
			"ghi<rst><c13><c9><ceos>abc xdef ghi<rst><c14><c9><ceos>abc xdef "
			"ghi<rst><c21>\r\n"
			"abc xdef ghi\r\n"
		)
	def test_next_word_key( self_ ):
		self_.check_scenario(
			"abc def ghi<home><c-right><c-right>x<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc "
			"<rst><gray><rst><c13><c9><ceos>abc d<rst><gray><rst><c14><c9><ceos>abc "
			"de<rst><gray><rst><c15><c9><ceos>abc def<rst><gray><rst><c16><c9><ceos>abc "
			"def <rst><gray><rst><c17><c9><ceos>abc def "
			"g<rst><gray><rst><c18><c9><ceos>abc def gh<rst><gray><rst><c19><c9><ceos>abc "
			"def ghi<rst><gray><rst><c20><c9><ceos>abc def ghi<rst><c9><c9><ceos>abc def "
			"ghi<rst><c12><c9><ceos>abc def ghi<rst><c16><c9><ceos>abc defx "
			"ghi<rst><c17><c9><ceos>abc defx ghi<rst><c21>\r\n"
			"abc defx ghi\r\n"
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
	def test_hint_scroll_up( self_ ):
		self_.check_scenario(
			"co<c-up><c-up><tab><cr><c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        "
			"<gray>color_green<rst><u3><c11><c9><ceos>co<rst><gray>lor_normal<rst>\r\n"
			"        <gray>co\r\n"
			"        <gray>color_black<rst>\r\n"
			"        "
			"<gray>color_red<rst><u3><c11><c9><ceos>co<rst><gray>lor_white<rst>\r\n"
			"        <gray>color_normal<rst>\r\n"
			"        <gray>co\r\n"
			"        "
			"<gray>color_black<rst><u3><c11><c9><ceos><white>color_white<rst><green><rst><c20><c9><ceos><white>color_white<rst><c20>\r\n"
			"color_white\r\n"
		)
	def test_history( self_ ):
		self_.check_scenario(
			"<up><up><up><up><down><down><down><down>four<cr><c-d>",
			"<c9><ceos>three<rst><gray><rst><c14><c9><ceos>two<rst><gray><rst><c12><c9><ceos>one<rst><gray><rst><c12><c9><ceos>two<rst><gray><rst><c12><c9><ceos>three<rst><gray><rst><c14><c9><ceos><rst><gray><rst><c9><c9><ceos>f<rst><gray><rst><c10><c9><ceos>fo<rst><gray><rst><c11><c9><ceos>fou<rst><gray><rst><c12><c9><ceos>four<rst><gray><rst><c13><c9><ceos>four<rst><c13>\r\n"
			"four\r\n"
		)
		with open( "replxx_history.txt", "rb" ) as f:
			self_.assertSequenceEqual( f.read().decode(), "one\ntwo\nthree\nfour\n" )
	def test_paren_matching( self_ ):
		self_.check_scenario(
			"ab(cd)ef<left><left><left><left><left><left><left><cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>ab<brightmagenta>(<rst><gray><rst><c12><c9><ceos>ab<brightmagenta>(<rst>c<rst><gray><rst><c13><c9><ceos>ab<brightmagenta>(<rst>cd<rst><gray><rst><c14><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst><gray><rst><c15><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>e<rst><gray><rst><c16><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><gray><rst><c17><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c16><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c15><c9><ceos>ab<brightred>(<rst>cd<brightmagenta>)<rst>ef<rst><c14><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c13><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c12><c9><ceos>ab<brightmagenta>(<rst>cd<brightred>)<rst>ef<rst><c11><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c10><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c17>\r\n"
			"ab(cd)ef\r\n"
		)
	def test_paren_not_matched( self_ ):
		self_.check_scenario(
			"a(b[c)d<left><left><left><left><left><left><left><cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>a<brightmagenta>(<rst><gray><rst><c11><c9><ceos>a<brightmagenta>(<rst>b<rst><gray><rst><c12><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst><gray><rst><c13><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<rst><gray><rst><c14><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst><gray><rst><c15><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><gray><rst><c16><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c15><c9><ceos>a<err>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c14><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c13><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c12><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c11><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<err>)<rst>d<rst><c10><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c9><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c16>\r\n"
			"a(b[c)d\r\n"
		)
	def test_tab_completion( self_ ):
		self_.check_scenario(
			"co<tab><tab>bri<tab>b<tab><cr><c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c11><c9><ceos>color_<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c15><c9><ceos>color_<rst><c15>\r\n"
			"<brightmagenta>color_<rst>black          "
			"<brightmagenta>color_<rst>cyan           "
			"<brightmagenta>color_<rst>brightblue\r\n"
			"<brightmagenta>color_<rst>red            "
			"<brightmagenta>color_<rst>lightgray      "
			"<brightmagenta>color_<rst>brightmagenta\r\n"
			"<brightmagenta>color_<rst>green          "
			"<brightmagenta>color_<rst>gray           "
			"<brightmagenta>color_<rst>brightcyan\r\n"
			"<brightmagenta>color_<rst>brown          "
			"<brightmagenta>color_<rst>brightred      <brightmagenta>color_<rst>white\r\n"
			"<brightmagenta>color_<rst>blue           "
			"<brightmagenta>color_<rst>brightgreen    <brightmagenta>color_<rst>normal\r\n"
			"<brightmagenta>color_<rst>magenta        <brightmagenta>color_<rst>yellow\r\n"
			"\x1b[1;32mreplxx<rst>> <c9><ceos>color_<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c15><c9><ceos>color_b<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        <gray>color_blue<rst><u3><c16><c9><ceos>color_br<rst><gray><rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        "
			"<gray>color_brightgreen<rst><u3><c17><c9><ceos>color_bri<rst><gray><rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        <gray>color_brightgreen<rst>\r\n"
			"        "
			"<gray>color_brightblue<rst><u3><c18><c9><ceos>color_bright<rst><gray><rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        <gray>color_brightgreen<rst>\r\n"
			"        "
			"<gray>color_brightblue<rst><u3><c21><c9><ceos>color_brightb<rst><green>lue<rst><c22><c9><ceos><brightblue>color_brightblue<rst><green><rst><c25><c9><ceos><brightblue>color_brightblue<rst><c25>\r\n"
			"color_brightblue\r\n"
		)

if __name__ == '__main__':
	unittest.main()

