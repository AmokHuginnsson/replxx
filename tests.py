#! /usr/bin/python3

import pexpect
import unittest
import re
import os
import subprocess
import signal
import time

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
	"<left>": "\033[D",
	"<aleft>": "\033OD",
	"<right>": "\033[C",
	"<aright>": "\033OC",
	"<up>": "\033[A",
	"<aup>": "\033OA",
	"<down>": "\033[B",
	"<adown>": "\033OB",
	"<c-left>": "\033[1;5D",
	"<c-right>": "\033[1;5C",
	"<c-up>": "\033[1;5A",
	"<c-down>": "\033[1;5B",
	"<m-left>": "\033[1;3D",
	"<m-right>": "\033[1;3C",
	"<m-up>": "\033[1;3A",
	"<m-down>": "\033[1;3B",
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
	"<c-t>": "",
	"<c-u>": "",
	"<c-v>": "",
	"<c-w>": "",
	"<c-y>": "",
	"<c-z>": "",
	"<m-b>": "\033b",
	"<m-c>": "\033c",
	"<m-d>": "\033d",
	"<m-f>": "\033f",
	"<m-l>": "\033l",
	"<m-n>": "\033n",
	"<m-p>": "\033p",
	"<m-u>": "\033u",
	"<m-y>": "\033y",
	"<m-backspace>": "\033\177",
	"<f1>": "\033OP",
	"<f2>": "\033OQ"
}

termseq = {
	"\x1bc": "<RIS>",
	"\x1b[0m": "<rst>",
	"\x1b[H": "<mvhm>",
	"\x1b[2J": "<clr>",
	"\x1b[J": "<ceos>",
	"\x1b[0;22;30m": "<black>",
	"\x1b[0;22;31m": "<red>",
	"\x1b[0;22;32m": "<green>",
	"\x1b[0;22;33m": "<brown>",
	"\x1b[0;22;34m": "<blue>",
	"\x1b[0;22;35m": "<magenta>",
	"\x1b[0;22;36m": "<cyan>",
	"\x1b[0;22;37m": "<lightgray>",
	"\x1b[0;1;30m": "<gray>",
	"\x1b[0;1;31m": "<brightred>",
	"\x1b[0;1;32m": "<brightgreen>",
	"\x1b[0;1;33m": "<yellow>",
	"\x1b[0;1;34m": "<brightblue>",
	"\x1b[0;1;35m": "<brightmagenta>",
	"\x1b[0;1;36m": "<brightcyan>",
	"\x1b[0;1;37m": "<white>",
	"\x1b[1;32m": "<brightgreen>",
	"\x1b[101;1;33m": "<err>",
	"\x07": "<bell>"
}
colRe = re.compile( "\\x1b\\[(\\d+)G" )
upRe = re.compile( "\\x1b\\[(\\d+)A" )
downRe = re.compile( "\\x1b\\[(\\d+)B" )

def sym_to_raw( str_ ):
	for sym, seq in keytab.items():
		str_ = str_.replace( sym, seq )
	return str_

def seq_to_sym( str_ ):
	for seq, sym in termseq.items():
		str_ = str_.replace( seq, sym )
	str_ = colRe.sub( "<c\\1>", str_ )
	str_ = upRe.sub( "<u\\1>", str_ )
	str_ = downRe.sub( "<d\\1>", str_ )
	return str_

_words_ = [
	"ada", "algol"
	"bash", "basic",
	"clojure", "cobol", "csharp",
	"eiffel", "erlang",
	"forth", "fortran", "fsharp",
	"go", "groovy",
	"haskell", "huginn",
	"java", "javascript", "julia",
	"kotlin",
	"lisp", "lua",
	"modula",
	"nemerle",
	"ocaml",
	"perl", "php", "prolog", "python",
	"rebol", "ruby", "rust",
	"scala", "scheme", "sql", "swift",
	"typescript"
]

def skip( test_ ):
	return "SKIP" in os.environ and os.environ["SKIP"].find( test_ ) >= 0

verbosity = None

class ReplxxTests( unittest.TestCase ):
	_prompt_ = "\033\\[1;32mreplxx\033\\[0m> "
	_cxxSample_ = "./build/example-cxx-api"
	_cSample_ = "./build/example-c-api"
	_end_ = "\r\nExiting Replxx\r\n"
	def check_scenario(
		self_, seq_, expected_,
		history = "one\ntwo\nthree\n",
		term = "xterm",
		command = _cxxSample_,
		dimensions = ( 25, 80 ),
		prompt = _prompt_,
		end = _prompt_ + _end_,
		encoding = "utf-8",
		pause = 0.25
	):
		with open( "replxx_history.txt", "wb" ) as f:
			f.write( history.encode( encoding ) )
			f.close()
		os.environ["TERM"] = term
		command = command.replace( "\n", "~" )
		if verbosity >= 2:
			print( "\nTERM: {}, SIZE: {}, CMD: {}".format( term, dimensions, command ) )
		prompt = prompt.replace( "\n", "\r\n" ).replace( "\r\r", "\r" )
		end = end.replace( "\n", "\r\n" ).replace( "\r\r", "\r" )
		self_._replxx = pexpect.spawn( command, maxread = 1, encoding = encoding, dimensions = dimensions )
		self_._replxx.expect( prompt )
		self_.maxDiff = None
		if isinstance( seq_, str ):
			seqs = seq_.split( "<c-z>" )
			for seq in seqs:
				last = seq is seqs[-1]
				if not last:
					seq += "<c-z>"
				self_._replxx.send( sym_to_raw( seq ) )
				if not last:
					time.sleep( pause )
					self_._replxx.kill( signal.SIGCONT )
		else:
			for seq in seq_:
				last = seq is seq_[-1]
				self_._replxx.send( sym_to_raw( seq ) )
				if not last:
					time.sleep( pause )
		self_._replxx.expect( end )
		self_.assertSequenceEqual( seq_to_sym( self_._replxx.before ), expected_ )
	def test_unicode( self_ ):
		self_.check_scenario(
			"<up><cr><c-d>",
			"<c9><ceos>aóą Ϩ 𓢀  󃔀  <rst><c21>"
			"<c9><ceos>aóą Ϩ 𓢀  󃔀  <rst><c21>\r\n"
			"aóą Ϩ 𓢀  󃔀  \r\n",
			"aóą Ϩ 𓢀  󃔀  \n"
		)
		self_.check_scenario(
			"aóą Ϩ 𓢀  󃔀  <cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>aó<rst><c11><c9><ceos>aóą<rst><c12><c9><ceos>aóą "
			"<rst><c13><c9><ceos>aóą Ϩ<rst><c14><c9><ceos>aóą Ϩ "
			"<rst><c15><c9><ceos>aóą Ϩ 𓢀<rst><c16><c9><ceos>aóą Ϩ 𓢀 "
			"<rst><c17><c9><ceos>aóą Ϩ 𓢀  "
			"<rst><c18><c9><ceos>aóą Ϩ 𓢀  󃔀<rst><c19><c9><ceos>aóą Ϩ 𓢀  󃔀 "
			"<rst><c20><c9><ceos>aóą Ϩ 𓢀  󃔀  "
			"<rst><c21><c9><ceos>aóą Ϩ 𓢀  󃔀  <rst><c21>\r\n"
			"aóą Ϩ 𓢀  󃔀  \r\n"
		)
	@unittest.skipIf( skip( "8bit_encoding" ), "broken platform" )
	def test_8bit_encoding( self_ ):
		LC_CTYPE = "LC_CTYPE"
		exists = LC_CTYPE in os.environ
		lcCtype = None
		if exists:
			lcCtype = os.environ[LC_CTYPE]
		os.environ[LC_CTYPE] = "pl_PL.ISO-8859-2"
		self_.check_scenario(
			"<aup><cr><c-d>",
			"<c9><ceos>text ~ó~<rst><c17><c9><ceos>text ~ó~<rst><c17>\r\ntext ~ó~\r\n",
			"text ~ó~\n",
			encoding = "iso-8859-2"
		)
		if exists:
			os.environ[LC_CTYPE] = lcCtype
		else:
			del os.environ[LC_CTYPE]
	def test_bad_term( self_ ):
		self_.check_scenario(
			"a line of text<cr><c-d>",
			"a line of text\r\na line of text\r\n",
			term = "dumb"
		)
	def test_ctrl_c( self_ ):
		self_.check_scenario(
			"abc<c-c><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abc<rst><c12>^C\r"
			"\r\n"
		)
	def test_ctrl_z( self_ ):
		self_.check_scenario(
			"<up><c-z><cr><c-d>",
			"<c9><ceos>three<rst><c14><brightgreen>replxx<rst>> "
			"<c9><ceos>three<rst><c14><c9><ceos>three<rst><c14>\r\n"
			"three\r\n"
		)
		self_.check_scenario(
			"<c-r>w<c-z><cr><c-d>",
			"<c1><ceos><c1><ceos>(reverse-i-search)`': "
			"<c23><c1><ceos>(reverse-i-search)`w': "
			"two<c25><c1><ceos>(reverse-i-search)`w': "
			"two<c25><c1><ceos><brightgreen>replxx<rst>> "
			"two<c10><c9><ceos>two<rst><c12>\r\n"
			"two\r\n"
		)
	def test_ctrl_l( self_ ):
		self_.check_scenario(
			"<cr><cr><cr><c-l><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <RIS><mvhm><clr><rst><brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9>",
			end = "\r\nExiting Replxx\r\n"
		)
		self_.check_scenario(
			"<cr><up><c-left><c-l><cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos>first "
			"second<rst><c21><c9><ceos>first "
			"second<rst><c15><RIS><mvhm><clr><rst><brightgreen>replxx<rst>> "
			"<c9><ceos>first second<rst><c15><c9><ceos>first second<rst><c21>\r\n"
			"first second\r\n",
			"first second\n"
		)
	def test_backspace( self_ ):
		self_.check_scenario(
			"<up><c-a><m-f><c-right><backspace><backspace><backspace><backspace><cr><c-d>",
			"<c9><ceos>one two three<rst><c22><c9><ceos>one two "
			"three<rst><c9><c9><ceos>one two three<rst><c12><c9><ceos>one two "
			"three<rst><c16><c9><ceos>one tw three<rst><c15><c9><ceos>one t "
			"three<rst><c14><c9><ceos>one  three<rst><c13><c9><ceos>one "
			"three<rst><c12><c9><ceos>one three<rst><c18>\r\n"
			"one three\r\n",
			"one two three\n"
		)
	def test_delete( self_ ):
		self_.check_scenario(
			"<up><m-b><c-left><del><c-d><del><c-d><cr><c-d>",
			"<c9><ceos>one two three<rst><c22><c9><ceos>one two "
			"three<rst><c17><c9><ceos>one two three<rst><c13><c9><ceos>one wo "
			"three<rst><c13><c9><ceos>one o three<rst><c13><c9><ceos>one  "
			"three<rst><c13><c9><ceos>one three<rst><c13><c9><ceos>one three<rst><c18>\r\n"
			"one three\r\n",
			"one two three\n"
		)
	def test_home_key( self_ ):
		self_.check_scenario(
			"abc<home>z<cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abc<rst><c9><c9><ceos>zabc<rst><c10><c9><ceos>zabc<rst><c13>\r\n"
			"zabc\r\n"
		)
	def test_end_key( self_ ):
		self_.check_scenario(
			"abc<home>z<end>q<cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abc<rst><c9><c9><ceos>zabc<rst><c10><c9><ceos>zabc<rst><c13><c9><ceos>zabcq<rst><c14><c9><ceos>zabcq<rst><c14>\r\n"
			"zabcq\r\n"
		)
	def test_left_key( self_ ):
		self_.check_scenario(
			"abc<left>x<aleft><left>y<cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abc<rst><c11><c9><ceos>abxc<rst><c12><c9><ceos>abxc<rst><c11><c9><ceos>abxc<rst><c10><c9><ceos>aybxc<rst><c11><c9><ceos>aybxc<rst><c14>\r\n"
			"aybxc\r\n"
		)
	def test_right_key( self_ ):
		self_.check_scenario(
			"abc<home><right>x<aright>y<cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abc<rst><c9><c9><ceos>abc<rst><c10><c9><ceos>axbc<rst><c11><c9><ceos>axbc<rst><c12><c9><ceos>axbyc<rst><c13><c9><ceos>axbyc<rst><c14>\r\n"
			"axbyc\r\n"
		)
	def test_prev_word_key( self_ ):
		self_.check_scenario(
			"abc def ghi<c-left><m-left>x<cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abc "
			"<rst><c13><c9><ceos>abc d<rst><c14><c9><ceos>abc "
			"de<rst><c15><c9><ceos>abc def<rst><c16><c9><ceos>abc "
			"def <rst><c17><c9><ceos>abc def "
			"g<rst><c18><c9><ceos>abc def gh<rst><c19><c9><ceos>abc "
			"def ghi<rst><c20><c9><ceos>abc def ghi<rst><c17><c9><ceos>abc def "
			"ghi<rst><c13><c9><ceos>abc xdef ghi<rst><c14><c9><ceos>abc xdef "
			"ghi<rst><c21>\r\n"
			"abc xdef ghi\r\n"
		)
	def test_next_word_key( self_ ):
		self_.check_scenario(
			"abc def ghi<home><c-right><m-right>x<cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abc "
			"<rst><c13><c9><ceos>abc d<rst><c14><c9><ceos>abc "
			"de<rst><c15><c9><ceos>abc def<rst><c16><c9><ceos>abc "
			"def <rst><c17><c9><ceos>abc def "
			"g<rst><c18><c9><ceos>abc def gh<rst><c19><c9><ceos>abc "
			"def ghi<rst><c20><c9><ceos>abc def ghi<rst><c9><c9><ceos>abc def "
			"ghi<rst><c12><c9><ceos>abc def ghi<rst><c16><c9><ceos>abc defx "
			"ghi<rst><c17><c9><ceos>abc defx ghi<rst><c21>\r\n"
			"abc defx ghi\r\n"
		)
	def test_hint_show( self_ ):
		self_.check_scenario(
			"co\r<c-d>",
			"<c9><ceos>c<rst><c10><c9><ceos>co<rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c11><c9><ceos>co<rst><c11>\r\n"
			"co\r\n"
		)
		self_.check_scenario(
			"<up><cr><c-d>",
			"<c9><ceos>zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz "
			"<brightgreen>color_brightgreen<rst><c15><u3><c9><ceos>zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz "
			"<brightgreen>color_brightgreen<rst><c15>\r\n"
			"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz color_brightgreen\r\n",
			"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz color_brightgreen\n",
			dimensions = ( 64, 16 )
		)
	def test_hint_scroll_down( self_ ):
		self_.check_scenario(
			"co<c-down><c-down><tab><cr><c-d>",
			"<c9><ceos>c<rst><c10><c9><ceos>co<rst>\r\n"
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
			"<gray>color_blue<rst><u3><c11><c9><ceos><red>color_red<rst><c18><c9><ceos><red>color_red<rst><c18>\r\n"
			"color_red\r\n"
		)
	def test_hint_scroll_up( self_ ):
		self_.check_scenario(
			"co<c-up><c-up><tab><cr><c-d>",
			"<c9><ceos>c<rst><c10><c9><ceos>co<rst>\r\n"
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
			"<gray>color_black<rst><u3><c11><c9><ceos><white>color_white<rst><c20><c9><ceos><white>color_white<rst><c20>\r\n"
			"color_white\r\n"
		)
	def test_history( self_ ):
		self_.check_scenario(
			"<up><up><up><up><down><down><down><down>four<cr><c-d>",
			"<c9><ceos>three<rst><c14><c9><ceos>two<rst><c12><c9><ceos>one<rst><c12><c9><ceos>two<rst><c12><c9><ceos>three<rst><c14><c9><ceos><rst><c9><c9><ceos>f<rst><c10><c9><ceos>fo<rst><c11><c9><ceos>fou<rst><c12><c9><ceos>four<rst><c13><c9><ceos>four<rst><c13>\r\n"
			"four\r\n"
		)
		with open( "replxx_history.txt", "rb" ) as f:
			self_.assertSequenceEqual( f.read().decode(), "one\ntwo\nthree\nfour\n" )
	def test_paren_matching( self_ ):
		self_.check_scenario(
			"ab(cd)ef<left><left><left><left><left><left><left><cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c9><ceos>ab<brightmagenta>(<rst><c12><c9><ceos>ab<brightmagenta>(<rst>c<rst><c13><c9><ceos>ab<brightmagenta>(<rst>cd<rst><c14><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst><c15><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>e<rst><c16><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c17><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c16><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c15><c9><ceos>ab<brightred>(<rst>cd<brightmagenta>)<rst>ef<rst><c14><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c13><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c12><c9><ceos>ab<brightmagenta>(<rst>cd<brightred>)<rst>ef<rst><c11><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c10><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c17>\r\n"
			"ab(cd)ef\r\n"
		)
	def test_paren_not_matched( self_ ):
		self_.check_scenario(
			"a(b[c)d<left><left><left><left><left><left><left><cr><c-d>",
			"<c9><ceos>a<rst><c10><c9><ceos>a<brightmagenta>(<rst><c11><c9><ceos>a<brightmagenta>(<rst>b<rst><c12><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst><c13><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<rst><c14><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst><c15><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c16><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c15><c9><ceos>a<err>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c14><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c13><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c12><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c11><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<err>)<rst>d<rst><c10><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c9><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c16>\r\n"
			"a(b[c)d\r\n"
		)
	def test_tab_completion( self_ ):
		self_.check_scenario(
			"co<tab><tab>bri<tab>b<tab><cr><c-d>",
			"<c9><ceos>c<rst><c10><c9><ceos>co<rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c11><c9><ceos>color_<rst>\r\n"
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
			"<brightgreen>replxx<rst>> <c9><ceos>color_<rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c15><c9><ceos>color_b<rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        <gray>color_blue<rst><u3><c16><c9><ceos>color_br<rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        "
			"<gray>color_brightgreen<rst><u3><c17><c9><ceos>color_bri<rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        <gray>color_brightgreen<rst>\r\n"
			"        "
			"<gray>color_brightblue<rst><u3><c18><c9><ceos>color_bright<rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        <gray>color_brightgreen<rst>\r\n"
			"        "
			"<gray>color_brightblue<rst><u3><c21><c9><ceos>color_brightb<rst><green>lue<rst><c22><c9><ceos><brightblue>color_brightblue<rst><c25><c9><ceos><brightblue>color_brightblue<rst><c25>\r\n"
			"color_brightblue\r\n"
		)
		self_.check_scenario(
			"<tab><tab>n<cr><c-d>",
			"<bell><bell><c9><ceos>n<rst><c10><c9><ceos>n<rst><c10>\r\nn\r\n",
			dimensions = ( 4, 32 ),
			command = ReplxxTests._cSample_ + " q1 e0"
		)
		self_.check_scenario(
			"<tab><tab>n<cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"db\r\n"
			"hello\r\n"
			"hallo\r\n"
			"--More--<bell>\r"
			"\t\t\t\t\r"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			dimensions = ( 4, 24 ),
			command = ReplxxTests._cSample_ + " q1 e1"
		)
		self_.check_scenario(
			"<up><home>co<tab><cr><c-d>",
			"<c9><ceos>abcd<brightmagenta>()<rst><c15>"
			"<c9><ceos>abcd<brightmagenta>()<rst><c9>"
			"<c9><ceos>cabcd<brightmagenta>()<rst><c10>"
			"<c9><ceos>coabcd<brightmagenta>()<rst><c11>"
			"<c9><ceos>color_abcd<brightmagenta>()<rst><c15>"
			"<c9><ceos>color_abcd<brightmagenta>()<rst><c21>\r\n"
			"color_abcd()\r\n",
			"abcd()\n"
		)
	def test_completion_shorter_result( self_ ):
		self_.check_scenario(
			"<up><tab><cr><c-d>",
			"<c9><ceos>\\pi<rst><c12><c9><ceos>π<rst><c10><c9><ceos>π<rst><c10>\r\n"
			"π\r\n",
			"\\pi\n"
		)
	def test_completion_pager( self_ ):
		cmd = ReplxxTests._cSample_ + " q1 x" + ",".join( _words_ )
		self_.check_scenario(
			"<tab>py<cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"ada         groovy      perl\r\n"
			"algolbash   haskell     php\r\n"
			"basic       huginn      prolog\r\n"
			"clojure     java        python\r\n"
			"cobol       javascript  rebol\r\n"
			"csharp      julia       ruby\r\n"
			"eiffel      kotlin      rust\r\n"
			"erlang      lisp        scala\r\n"
			"forth       lua         scheme\r\n"
			"--More--<bell>\r"
			"\t\t\t\t\r"
			"fortran     modula      sql\r\n"
			"fsharp      nemerle     swift\r\n"
			"go          ocaml       typescript\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			dimensions = ( 10, 40 ),
			command = cmd
		)
		self_.check_scenario(
			"<tab><cr><cr><cr><cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"ada         groovy      perl\r\n"
			"algolbash   haskell     php\r\n"
			"basic       huginn      prolog\r\n"
			"clojure     java        python\r\n"
			"cobol       javascript  rebol\r\n"
			"csharp      julia       ruby\r\n"
			"eiffel      kotlin      rust\r\n"
			"erlang      lisp        scala\r\n"
			"forth       lua         scheme\r\n"
			"--More--\r"
			"\t\t\t\t\r"
			"fortran     modula      sql\r\n"
			"--More--\r"
			"\t\t\t\t\r"
			"fsharp      nemerle     swift\r\n"
			"--More--\r"
			"\t\t\t\t\r"
			"go          ocaml       typescript\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			dimensions = ( 10, 40 ),
			command = cmd
		)
		self_.check_scenario(
			"<tab><c-c><cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"ada         kotlin\r\n"
			"algolbash   lisp\r\n"
			"basic       lua\r\n"
			"clojure     modula\r\n"
			"cobol       nemerle\r\n"
			"csharp      ocaml\r\n"
			"eiffel      perl\r\n"
			"--More--^C\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			dimensions = ( 8, 32 ),
			command = cmd
		)
		self_.check_scenario(
			"<tab>q<cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"ada         kotlin\r\n"
			"algolbash   lisp\r\n"
			"basic       lua\r\n"
			"clojure     modula\r\n"
			"cobol       nemerle\r\n"
			"csharp      ocaml\r\n"
			"eiffel      perl\r\n"
			"--More--\r"
			"\t\t\t\t\r"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			dimensions = ( 8, 32 ),
			command = cmd
		)
	def test_double_tab_completion( self_ ):
		cmd = ReplxxTests._cSample_ + " d1 q1 x" + ",".join( _words_ )
		self_.check_scenario(
			"fo<tab><tab>r<tab><cr><c-d>",
			"<c9><ceos>f<rst>\r\n"
			"        <gray>forth<rst>\r\n"
			"        <gray>fortran<rst>\r\n"
			"        <gray>fsharp<rst><u3><c10><c9><ceos>fo<rst>\r\n"
			"        <gray>forth<rst>\r\n"
			"        <gray>fortran<rst><u2><c11><c9><ceos>fort<rst>\r\n"
			"        <gray>forth<rst>\r\n"
			"        "
			"<gray>fortran<rst><u2><c13><c9><ceos>fortr<rst><gray>an<rst><c14><c9><ceos>fortran<rst><c16><c9><ceos>fortran<rst><c16>\r\n"
			"fortran\r\n",
			command = cmd
		)
	def test_beep_on_ambiguous_completion( self_ ):
		cmd = ReplxxTests._cSample_ + " b1 d1 q1 x" + ",".join( _words_ )
		self_.check_scenario(
			"fo<tab><tab>r<tab><cr><c-d>",
			"<c9><ceos>f<rst>\r\n"
			"        <gray>forth<rst>\r\n"
			"        <gray>fortran<rst>\r\n"
			"        <gray>fsharp<rst><u3><c10><c9><ceos>fo<rst>\r\n"
			"        <gray>forth<rst>\r\n"
			"        <gray>fortran<rst><u2><c11><bell><c9><ceos>fort<rst>\r\n"
			"        <gray>forth<rst>\r\n"
			"        "
			"<gray>fortran<rst><u2><c13><bell><c9><ceos>fortr<rst><gray>an<rst><c14><c9><ceos>fortran<rst><c16><c9><ceos>fortran<rst><c16>\r\n"
			"fortran\r\n",
			command = cmd
		)
	def test_history_search_backward( self_ ):
		self_.check_scenario(
			"<c-r>repl<c-r><cr><c-d>",
			"<c1><ceos><c1><ceos>(reverse-i-search)`': "
			"<c23><c1><ceos>(reverse-i-search)`r': echo repl "
			"golf<c29><c1><ceos>(reverse-i-search)`re': echo repl "
			"golf<c30><c1><ceos>(reverse-i-search)`rep': echo repl "
			"golf<c31><c1><ceos>(reverse-i-search)`repl': echo repl "
			"golf<c32><c1><ceos>(reverse-i-search)`repl': charlie repl "
			"delta<c35><c1><ceos><brightgreen>replxx<rst>> charlie repl "
			"delta<c17><c9><ceos>charlie repl delta<rst><c27>\r\n"
			"charlie repl delta\r\n",
			"some command\n"
			"alfa repl bravo\n"
			"other request\n"
			"charlie repl delta\n"
			"misc input\n"
			"echo repl golf\n"
			"final thoughts\n"
		)
		self_.check_scenario(
			"<c-r>for<backspace><backspace>s<cr><c-d>",
			"<c1><ceos><c1><ceos>(reverse-i-search)`': "
			"<c23><c1><ceos>(reverse-i-search)`f': "
			"swift<c27><c1><ceos>(reverse-i-search)`fo': "
			"fortran<c25><c1><ceos>(reverse-i-search)`for': "
			"fortran<c26><c1><ceos>(reverse-i-search)`fo': "
			"fortran<c25><c1><ceos>(reverse-i-search)`f': "
			"swift<c27><c1><ceos>(reverse-i-search)`fs': "
			"fsharp<c25><c1><ceos><brightgreen>replxx<rst>> "
			"fsharp<c9><c9><ceos>fsharp<rst><c15>\r\n"
			"fsharp\r\n",
			"\n".join( _words_ ) + "\n"
		)
		self_.check_scenario(
			"<c-r>mod<c-l><cr><c-d>",
			"<c1><ceos><c1><ceos>(reverse-i-search)`': "
			"<c23><c1><ceos>(reverse-i-search)`m': "
			"scheme<c28><c1><ceos>(reverse-i-search)`mo': "
			"modula<c25><c1><ceos>(reverse-i-search)`mod': "
			"modula<c26><c1><ceos><brightgreen>replxx<rst>> "
			"<c9><RIS><mvhm><clr><rst><brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			"\n".join( _words_ ) + "\n"
		)
	def test_history_prefix_search_backward( self_ ):
		self_.check_scenario(
			"repl<m-p><m-p><cr><c-d>",
			"<c9><ceos>r<rst><c10><c9><ceos>re<rst><c11><c9><ceos>rep<rst><c12><c9><ceos>repl<rst><c13><c9><ceos>repl_echo "
			"golf<rst><c23><c9><ceos>repl_charlie "
			"delta<rst><c27><c9><ceos>repl_charlie delta<rst><c27>\r\n"
			"repl_charlie delta\r\n",
			"some command\n"
			"repl_alfa bravo\n"
			"other request\n"
			"repl_charlie delta\n"
			"misc input\n"
			"repl_echo golf\n"
			"final thoughts\n"
		)
	def test_history_browse( self_ ):
		self_.check_scenario(
			"<up><aup><pgup><down><up><up><adown><pgdown><up><down><down><up><cr><c-d>",
			"<c9><ceos>twelve<rst><c15>"
			"<c9><ceos>eleven<rst><c15>"
			"<c9><ceos>one<rst><c12>"
			"<c9><ceos>two<rst><c12>"
			"<c9><ceos>one<rst><c12>"
			"<c9><ceos>two<rst><c12>"
			"<c9><ceos><rst><c9>"
			"<c9><ceos>twelve<rst><c15>"
			"<c9><ceos><rst><c9>"
			"<c9><ceos>twelve<rst><c15>"
			"<c9><ceos>twelve<rst><c15>\r\n"
			"twelve\r\n",
			"one\n"
			"two\n"
			"three\n"
			"four\n"
			"five\n"
			"six\n"
			"seven\n"
			"eight\n"
			"nine\n"
			"ten\n"
			"eleven\n"
			"twelve\n"
		)
	def test_history_max_size( self_ ):
		self_.check_scenario(
			"<pgup><pgdown>a<cr><pgup><cr><c-d>",
			"<c9><ceos>three<rst><c14><c9><ceos><rst><c9><c9><ceos>a<rst><c10><c9><ceos>a<rst><c10>\r\n"
			"a\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos>four<rst><c13><c9><ceos>four<rst><c13>\r\n"
			"four\r\n",
			"one\n"
			"two\n"
			"three\n"
			"four\n"
			"five\n",
			command = ReplxxTests._cSample_ + " q1 s3"
		)
	def test_capitalize( self_ ):
		self_.check_scenario(
			"<up><home><right><m-c><m-c><right><right><m-c><m-c><m-c><cr><c-d>",
			"<c9><ceos>abc defg ijklmn zzxq<rst><c29><c9><ceos>abc defg ijklmn "
			"zzxq<rst><c9><c9><ceos>abc defg ijklmn zzxq<rst><c10><c9><ceos>aBc defg "
			"ijklmn zzxq<rst><c12><c9><ceos>aBc Defg ijklmn zzxq<rst><c17><c9><ceos>aBc "
			"Defg ijklmn zzxq<rst><c18><c9><ceos>aBc Defg ijklmn "
			"zzxq<rst><c19><c9><ceos>aBc Defg iJklmn zzxq<rst><c24><c9><ceos>aBc Defg "
			"iJklmn Zzxq<rst><c29><c9><ceos>aBc Defg iJklmn Zzxq<rst><c29>\r\n"
			"aBc Defg iJklmn Zzxq\r\n",
			"abc defg ijklmn zzxq\n"
		)
	def test_make_upper_case( self_ ):
		self_.check_scenario(
			"<up><home><right><right><right><m-u><m-u><right><m-u><cr><c-d>",
			"<c9><ceos>abcdefg hijklmno pqrstuvw<rst><c34><c9><ceos>abcdefg "
			"hijklmno pqrstuvw<rst><c9><c9><ceos>abcdefg hijklmno "
			"pqrstuvw<rst><c10><c9><ceos>abcdefg hijklmno "
			"pqrstuvw<rst><c11><c9><ceos>abcdefg hijklmno "
			"pqrstuvw<rst><c12><c9><ceos>abcDEFG hijklmno "
			"pqrstuvw<rst><c16><c9><ceos>abcDEFG HIJKLMNO "
			"pqrstuvw<rst><c25><c9><ceos>abcDEFG HIJKLMNO "
			"pqrstuvw<rst><c26><c9><ceos>abcDEFG HIJKLMNO "
			"PQRSTUVW<rst><c34><c9><ceos>abcDEFG HIJKLMNO "
			"PQRSTUVW<rst><c34>\r\n"
			"abcDEFG HIJKLMNO PQRSTUVW\r\n",
			"abcdefg hijklmno pqrstuvw\n"
		)
	def test_make_lower_case( self_ ):
		self_.check_scenario(
			"<up><home><right><right><right><m-l><m-l><right><m-l><cr><c-d>",
			"<c9><ceos>ABCDEFG HIJKLMNO PQRSTUVW<rst><c34><c9><ceos>ABCDEFG "
			"HIJKLMNO PQRSTUVW<rst><c9><c9><ceos>ABCDEFG HIJKLMNO "
			"PQRSTUVW<rst><c10><c9><ceos>ABCDEFG HIJKLMNO "
			"PQRSTUVW<rst><c11><c9><ceos>ABCDEFG HIJKLMNO "
			"PQRSTUVW<rst><c12><c9><ceos>ABCdefg HIJKLMNO "
			"PQRSTUVW<rst><c16><c9><ceos>ABCdefg hijklmno "
			"PQRSTUVW<rst><c25><c9><ceos>ABCdefg hijklmno "
			"PQRSTUVW<rst><c26><c9><ceos>ABCdefg hijklmno "
			"pqrstuvw<rst><c34><c9><ceos>ABCdefg hijklmno "
			"pqrstuvw<rst><c34>\r\n"
			"ABCdefg hijklmno pqrstuvw\r\n",
			"ABCDEFG HIJKLMNO PQRSTUVW\n"
		)
	def test_transpose( self_ ):
		self_.check_scenario(
			"<up><home><c-t><right><c-t><c-t><c-t><c-t><c-t><cr><c-d>",
			"<c9><ceos>abcd<rst><c13>"
			"<c9><ceos>abcd<rst><c9>"
			"<c9><ceos>abcd<rst><c10>"
			"<c9><ceos>bacd<rst><c11>"
			"<c9><ceos>bcad<rst><c12>"
			"<c9><ceos>bcda<rst><c13>"
			"<c9><ceos>bcad<rst><c13>"
			"<c9><ceos>bcda<rst><c13>"
			"<c9><ceos>bcda<rst><c13>\r\n"
			"bcda\r\n",
			"abcd\n"
		)
	def test_kill_to_beginning_of_line( self_ ):
		self_.check_scenario(
			"<up><home><c-right><c-right><right><c-u><end><c-y><cr><c-d>",
			"<c9><ceos><brightblue>+<rst>abc defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c32><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c9><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c13><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c18><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c19><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>+<rst><c9><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>+<rst><c22><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc "
			"defg<brightblue>-<rst><c32><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc defg<brightblue>-<rst><c32>\r\n"
			"-ijklmn zzxq++abc defg-\r\n",
			"+abc defg--ijklmn zzxq+\n"
		)
	def test_kill_to_end_of_line( self_ ):
		self_.check_scenario(
			"<up><home><c-right><c-right><right><c-k><home><c-y><cr><c-d>",
			"<c9><ceos><brightblue>+<rst>abc defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c32><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c9><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c13><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c18><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c19><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>-<rst><c19><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>-<rst><c9><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc "
			"defg<brightblue>-<rst><c22><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc defg<brightblue>-<rst><c32>\r\n"
			"-ijklmn zzxq++abc defg-\r\n",
			"+abc defg--ijklmn zzxq+\n"
		)
	def test_kill_next_word( self_ ):
		self_.check_scenario(
			"<up><home><c-right><m-d><c-right><c-y><cr><c-d>",
			"<c9><ceos>alpha charlie bravo delta<rst><c34><c9><ceos>alpha "
			"charlie bravo delta<rst><c9><c9><ceos>alpha charlie bravo "
			"delta<rst><c14><c9><ceos>alpha bravo delta<rst><c14><c9><ceos>alpha bravo "
			"delta<rst><c20><c9><ceos>alpha bravo charlie delta<rst><c28><c9><ceos>alpha "
			"bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"alpha charlie bravo delta\n"
		)
	def test_kill_prev_word_to_white_space( self_ ):
		self_.check_scenario(
			"<up><c-left><c-w><c-left><c-y><cr><c-d>",
			"<c9><ceos>alpha charlie bravo delta<rst><c34><c9><ceos>alpha "
			"charlie bravo delta<rst><c29><c9><ceos>alpha charlie "
			"delta<rst><c23><c9><ceos>alpha charlie delta<rst><c15><c9><ceos>alpha bravo "
			"charlie delta<rst><c21><c9><ceos>alpha bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"alpha charlie bravo delta\n"
		)
	def test_kill_prev_word( self_ ):
		self_.check_scenario(
			"<up><c-left><m-backspace><c-left><c-y><cr><c-d>",
			"<c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"bravo<brightmagenta>.<rst>delta<rst><c34><c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"bravo<brightmagenta>.<rst>delta<rst><c29><c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"delta<rst><c23><c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"delta<rst><c15><c9><ceos>alpha<brightmagenta>.<rst>bravo<brightmagenta>.<rst>charlie "
			"delta<rst><c21><c9><ceos>alpha<brightmagenta>.<rst>bravo<brightmagenta>.<rst>charlie "
			"delta<rst><c34>\r\n"
			"alpha.bravo.charlie delta\r\n",
			"alpha.charlie bravo.delta\n"
		)
	def test_kill_ring( self_ ):
		self_.check_scenario(
			"<up><c-w><backspace><c-w><backspace><c-w><backspace><c-u><c-y><m-y><m-y><m-y> <c-y><m-y><m-y><m-y> <c-y><m-y><m-y><m-y> <c-y><m-y><m-y><m-y><cr><c-d>",
			"<c9><ceos>delta charlie bravo alpha<rst><c34><c9><ceos>delta "
			"charlie bravo <rst><c29><c9><ceos>delta charlie "
			"bravo<rst><c28><c9><ceos>delta charlie "
			"<rst><c23><c9><ceos>delta "
			"charlie<rst><c22><c9><ceos>delta "
			"<rst><c15>"
			"<c9><ceos>delta<rst><c14>"
			"<c9><ceos><rst><c9>"
			"<c9><ceos>delta<rst><c14>"
			"<c9><ceos>charlie<rst><c16>"
			"<c9><ceos>bravo<rst><c14>"
			"<c9><ceos>alpha<rst><c14>"
			"<c9><ceos>alpha "
			"<rst><c15><c9><ceos>alpha "
			"alpha<rst><c20><c9><ceos>alpha "
			"delta<rst><c20><c9><ceos>alpha "
			"charlie<rst><c22><c9><ceos>alpha "
			"bravo<rst><c20><c9><ceos>alpha bravo "
			"<rst><c21><c9><ceos>alpha bravo "
			"bravo<rst><c26><c9><ceos>alpha bravo "
			"alpha<rst><c26><c9><ceos>alpha bravo "
			"delta<rst><c26><c9><ceos>alpha bravo "
			"charlie<rst><c28><c9><ceos>alpha bravo charlie "
			"<rst><c29><c9><ceos>alpha bravo charlie "
			"charlie<rst><c36><c9><ceos>alpha bravo charlie "
			"bravo<rst><c34><c9><ceos>alpha bravo charlie "
			"alpha<rst><c34><c9><ceos>alpha bravo charlie "
			"delta<rst><c34><c9><ceos>alpha bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"delta charlie bravo alpha\n"
		)
		self_.check_scenario(
			"<up><c-w><c-w><backspace><c-a><c-y> <cr><c-d>",
			"<c9><ceos>charlie delta alpha bravo<rst><c34><c9><ceos>charlie "
			"delta alpha <rst><c29><c9><ceos>charlie delta "
			"<rst><c23><c9><ceos>charlie "
			"delta<rst><c22><c9><ceos>charlie delta<rst><c9><c9><ceos>alpha "
			"bravocharlie delta<rst><c20><c9><ceos>alpha bravo charlie "
			"delta<rst><c21><c9><ceos>alpha bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"charlie delta alpha bravo\n"
		)
		self_.check_scenario(
			"<up><home><m-d><m-d><del><c-e> <c-y><cr><c-d>",
			"<c9><ceos>charlie delta alpha bravo<rst><c34><c9><ceos>charlie "
			"delta alpha bravo<rst><c9><c9><ceos> delta alpha bravo<rst><c9><c9><ceos> "
			"alpha bravo<rst><c9><c9><ceos>alpha bravo<rst><c9><c9><ceos>alpha "
			"bravo<rst><c20><c9><ceos>alpha bravo "
			"<rst><c21><c9><ceos>alpha bravo charlie "
			"delta<rst><c34><c9><ceos>alpha bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"charlie delta alpha bravo\n"
		)
		self_.check_scenario(
			"<up><c-w><backspace><c-w><backspace><c-w><backspace><c-w><backspace><c-w><backspace>"
			"<c-w><backspace><c-w><backspace><c-w><backspace><c-w><backspace><c-w><backspace>"
			"<c-w><c-y><m-y><m-y><m-y><m-y><m-y><m-y><m-y><m-y><m-y><m-y><cr><c-d>",
			"<c9><ceos>a b c d e f g h i j k<rst><c30><c9><ceos>a b c d e f g "
			"h i j <rst><c29><c9><ceos>a b c d e f g h i "
			"j<rst><c28><c9><ceos>a b c d e f g h i "
			"<rst><c27><c9><ceos>a b c d e f g h "
			"i<rst><c26><c9><ceos>a b c d e f g h "
			"<rst><c25><c9><ceos>a b c d e f g "
			"h<rst><c24><c9><ceos>a b c d e f g "
			"<rst><c23><c9><ceos>a b c d e f g<rst><c22><c9><ceos>a "
			"b c d e f <rst><c21><c9><ceos>a b c d e "
			"f<rst><c20><c9><ceos>a b c d e <rst><c19><c9><ceos>a b "
			"c d e<rst><c18><c9><ceos>a b c d <rst><c17><c9><ceos>a "
			"b c d<rst><c16><c9><ceos>a b c <rst><c15><c9><ceos>a b "
			"c<rst><c14><c9><ceos>a b <rst><c13><c9><ceos>a "
			"b<rst><c12><c9><ceos>a "
			"<rst><c11>"
			"<c9><ceos>a<rst><c10>"
			"<c9><ceos><rst><c9>"
			"<c9><ceos>a<rst><c10>"
			"<c9><ceos>b<rst><c10>"
			"<c9><ceos>c<rst><c10>"
			"<c9><ceos>d<rst><c10>"
			"<c9><ceos>e<rst><c10>"
			"<c9><ceos>f<rst><c10>"
			"<c9><ceos>g<rst><c10>"
			"<c9><ceos>h<rst><c10>"
			"<c9><ceos>i<rst><c10>"
			"<c9><ceos>j<rst><c10>"
			"<c9><ceos>a<rst><c10>"
			"<c9><ceos>a<rst><c10>\r\n"
			"a\r\n",
			"a b c d e f g h i j k\n"
		)
	def test_tab_completion_cutoff( self_ ):
		self_.check_scenario(
			"<tab>n<tab>y<cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"Display all 9 possibilities? (y or n)\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n"
			"Display all 9 possibilities? (y or n)<ceos>\r\n"
			"db            hallo         hansekogge    quetzalcoatl  power\r\n"
			"hello         hans          seamann       quit\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			command = ReplxxTests._cSample_ + " q1 c3"
		)
		self_.check_scenario(
			"<tab>n<cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"Display all 9 possibilities? (y or n)\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			command = ReplxxTests._cSample_ + " q1 c3"
		)
		self_.check_scenario(
			"<tab><c-c><cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"Display all 9 possibilities? (y or n)^C\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos><rst><c9>\r\n",
			command = ReplxxTests._cSample_ + " q1 c3"
		)
	def test_preload( self_ ):
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>Alice has a cat.<rst><c25>"
			"<c9><ceos>Alice has a cat.<rst><c25>\r\n"
			"Alice has a cat.\r\n",
			command = ReplxxTests._cSample_ + " q1 'iAlice has a cat.'"
		)
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>Cat  eats  mice. "
			"<rst><c26><c9><ceos>Cat  eats  mice. "
			"<rst><c26>\r\n"
			"Cat  eats  mice. "
			"\r\n",
			command = ReplxxTests._cSample_ + " q1 'iCat\teats\tmice.\r\n'"
		)
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>Cat  eats  mice. "
			"<rst><c26><c9><ceos>Cat  eats  mice. "
			"<rst><c26>\r\n"
			"Cat  eats  mice. "
			"\r\n",
			command = ReplxxTests._cSample_ + " q1 'iCat\teats\tmice.\r\n\r\n\n\n'"
		)
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>M Alice has a cat.<rst><c27>"
			"<c9><ceos>M Alice has a cat.<rst><c27>\r\n"
			"M Alice has a cat.\r\n",
			command = ReplxxTests._cSample_ + " q1 'iMAlice has a cat.'"
		)
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>M  Alice has a cat.<rst><c28>"
			"<c9><ceos>M  Alice has a cat.<rst><c28>\r\n"
			"M  Alice has a cat.\r\n",
			command = ReplxxTests._cSample_ + " q1 'iM\t\t\t\tAlice has a cat.'"
		)
	def test_prompt( self_ ):
		prompt = "date: now\nrepl> "
		self_.check_scenario(
			"<up><cr><up><up><cr><c-d>",
			"<c7><ceos>three<rst><c12><c7><ceos>three<rst><c12>\r\n"
			"three\r\n"
			"date: now\r\n"
			"repl> "
			"<c7><ceos>three<rst><c12><c7><ceos>two<rst><c10><c7><ceos>two<rst><c10>\r\n"
			"two\r\n",
			command = ReplxxTests._cSample_ + " q1 'p{}'".format( prompt ),
			prompt = prompt,
			end = prompt + ReplxxTests._end_
		)
	def test_long_line( self_ ):
		self_.check_scenario(
			"<up><c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left>~<c-left><cr><c-d>",
			"<c9><ceos>ada clojure eiffel fortran groovy java kotlin modula perl python "
			"rust sql<rst><c2><u2><c9><ceos>ada clojure eiffel fortran groovy "
			"java kotlin modula perl python rust sql<rst><u1><c39><u1><c9><ceos>ada "
			"clojure eiffel fortran groovy java kotlin modula perl python rust "
			"~sql<rst><u1><c40><u1><c9><ceos>ada clojure eiffel fortran groovy java "
			"kotlin modula perl python rust ~sql<rst><u1><c34><u1><c9><ceos>ada clojure "
			"eiffel fortran groovy java kotlin modula perl python ~rust "
			"~sql<rst><u1><c35><u1><c9><ceos>ada clojure eiffel fortran groovy java "
			"kotlin modula perl python ~rust ~sql<rst><u1><c27><u1><c9><ceos>ada clojure "
			"eiffel fortran groovy java kotlin modula perl ~python ~rust "
			"~sql<rst><u1><c28><u1><c9><ceos>ada clojure eiffel fortran groovy java "
			"kotlin modula perl ~python ~rust ~sql<rst><u1><c22><u1><c9><ceos>ada clojure "
			"eiffel fortran groovy java kotlin modula ~perl ~python ~rust "
			"~sql<rst><u1><c23><u1><c9><ceos>ada clojure eiffel fortran groovy java "
			"kotlin modula ~perl ~python ~rust ~sql<rst><u1><c15><u1><c9><ceos>ada "
			"clojure eiffel fortran groovy java kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u1><c16><u1><c9><ceos>ada clojure eiffel fortran groovy java "
			"kotlin ~modula ~perl ~python ~rust ~sql<rst><u1><c8><u1><c9><ceos>ada "
			"clojure eiffel fortran groovy java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u1><c9><u1><c9><ceos>ada clojure eiffel fortran groovy java "
			"~kotlin ~modula ~perl ~python ~rust ~sql<rst><u1><c3><u1><c9><ceos>ada "
			"clojure eiffel fortran groovy ~java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u1><c4><u1><c9><ceos>ada clojure eiffel fortran groovy ~java "
			"~kotlin ~modula ~perl ~python ~rust ~sql<rst><u2><c36><c9><ceos>ada clojure "
			"eiffel fortran ~groovy ~java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u2><c37><c9><ceos>ada clojure eiffel fortran ~groovy ~java ~kotlin "
			"~modula ~perl ~python ~rust ~sql<rst><u2><c28><c9><ceos>ada clojure eiffel "
			"~fortran ~groovy ~java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u2><c29><c9><ceos>ada clojure eiffel ~fortran ~groovy ~java "
			"~kotlin ~modula ~perl ~python ~rust ~sql<rst><u2><c21><c9><ceos>ada clojure "
			"~eiffel ~fortran ~groovy ~java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u2><c22><c9><ceos>ada clojure ~eiffel ~fortran ~groovy ~java "
			"~kotlin ~modula ~perl ~python ~rust ~sql<rst><u2><c13><c9><ceos>ada ~clojure "
			"~eiffel ~fortran ~groovy ~java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u2><c14><c9><ceos>ada ~clojure ~eiffel ~fortran ~groovy ~java "
			"~kotlin ~modula ~perl ~python ~rust ~sql<rst><u2><c9><c9><ceos>~ada ~clojure "
			"~eiffel ~fortran ~groovy ~java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><u2><c10><c9><ceos>~ada ~clojure ~eiffel ~fortran ~groovy ~java "
			"~kotlin ~modula ~perl ~python ~rust ~sql<rst><u2><c9><c9><ceos>~ada ~clojure "
			"~eiffel ~fortran ~groovy ~java ~kotlin ~modula ~perl ~python ~rust "
			"~sql<rst><c14>\r\n"
			"~ada ~clojure ~eiffel ~fortran ~groovy ~java ~kotlin ~modula ~perl ~python "
			"~rust ~sql\r\n",
			" ".join( _words_[::3] ) + "\n",
			dimensions = ( 10, 40 )
		)
	def test_colors( self_ ):
		self_.check_scenario(
			"<up><cr><c-d>",
			"<c9><ceos><black>color_black<rst> <red>color_red<rst> "
			"<green>color_green<rst> <brown>color_brown<rst> <blue>color_blue<rst> "
			"<magenta>color_magenta<rst> <cyan>color_cyan<rst> "
			"<lightgray>color_lightgray<rst> <gray>color_gray<rst> "
			"<brightred>color_brightred<rst> <brightgreen>color_brightgreen<rst> "
			"<yellow>color_yellow<rst> <brightblue>color_brightblue<rst> "
			"<brightmagenta>color_brightmagenta<rst> <brightcyan>color_brightcyan<rst> "
			"<white>color_white<rst><c70><u2><c9><ceos><black>color_black<rst> "
			"<red>color_red<rst> <green>color_green<rst> <brown>color_brown<rst> "
			"<blue>color_blue<rst> <magenta>color_magenta<rst> <cyan>color_cyan<rst> "
			"<lightgray>color_lightgray<rst> <gray>color_gray<rst> "
			"<brightred>color_brightred<rst> <brightgreen>color_brightgreen<rst> "
			"<yellow>color_yellow<rst> <brightblue>color_brightblue<rst> "
			"<brightmagenta>color_brightmagenta<rst> <brightcyan>color_brightcyan<rst> "
			"<white>color_white<rst><c70>\r\n"
			"color_black color_red color_green color_brown color_blue color_magenta "
			"color_cyan color_lightgray color_gray color_brightred color_brightgreen "
			"color_yellow color_brightblue color_brightmagenta color_brightcyan "
			"color_white\r\n",
			"color_black color_red color_green color_brown color_blue color_magenta color_cyan color_lightgray"
			" color_gray color_brightred color_brightgreen color_yellow color_brightblue color_brightmagenta color_brightcyan color_white\n"
		)
	def test_word_break_characters( self_ ):
		self_.check_scenario(
			"<up><c-left>x<c-left><c-left>x<c-left><c-left>x<c-left><c-left>x<c-left><c-left>x<c-left><c-left>x<cr><c-d>",
			"<c9><ceos>one_two three-four five_six "
			"seven-eight<rst><c48><c9><ceos>one_two three-four five_six "
			"seven-eight<rst><c43><c9><ceos>one_two three-four five_six "
			"seven-xeight<rst><c44><c9><ceos>one_two three-four five_six "
			"seven-xeight<rst><c43><c9><ceos>one_two three-four five_six "
			"seven-xeight<rst><c37><c9><ceos>one_two three-four five_six "
			"xseven-xeight<rst><c38><c9><ceos>one_two three-four five_six "
			"xseven-xeight<rst><c37><c9><ceos>one_two three-four five_six "
			"xseven-xeight<rst><c28><c9><ceos>one_two three-four xfive_six "
			"xseven-xeight<rst><c29><c9><ceos>one_two three-four xfive_six "
			"xseven-xeight<rst><c28><c9><ceos>one_two three-four xfive_six "
			"xseven-xeight<rst><c23><c9><ceos>one_two three-xfour xfive_six "
			"xseven-xeight<rst><c24><c9><ceos>one_two three-xfour xfive_six "
			"xseven-xeight<rst><c23><c9><ceos>one_two three-xfour xfive_six "
			"xseven-xeight<rst><c17><c9><ceos>one_two xthree-xfour xfive_six "
			"xseven-xeight<rst><c18><c9><ceos>one_two xthree-xfour xfive_six "
			"xseven-xeight<rst><c17><c9><ceos>one_two xthree-xfour xfive_six "
			"xseven-xeight<rst><c9><c9><ceos>xone_two xthree-xfour xfive_six "
			"xseven-xeight<rst><c10><c9><ceos>xone_two xthree-xfour xfive_six "
			"xseven-xeight<rst><c54>\r\n"
			"xone_two xthree-xfour xfive_six xseven-xeight\r\n",
			"one_two three-four five_six seven-eight\n",
			command = ReplxxTests._cSample_ + " q1 'w \t-'"
		)
		self_.check_scenario(
			"<up><c-left>x<c-left><c-left>x<c-left><c-left>x<c-left><c-left>x<c-left><c-left>x<c-left><c-left>x<cr><c-d>",
			"<c9><ceos>one_two three-four five_six "
			"seven-eight<rst><c48><c9><ceos>one_two three-four five_six "
			"seven-eight<rst><c37><c9><ceos>one_two three-four five_six "
			"xseven-eight<rst><c38><c9><ceos>one_two three-four five_six "
			"xseven-eight<rst><c37><c9><ceos>one_two three-four five_six "
			"xseven-eight<rst><c33><c9><ceos>one_two three-four five_xsix "
			"xseven-eight<rst><c34><c9><ceos>one_two three-four five_xsix "
			"xseven-eight<rst><c33><c9><ceos>one_two three-four five_xsix "
			"xseven-eight<rst><c28><c9><ceos>one_two three-four xfive_xsix "
			"xseven-eight<rst><c29><c9><ceos>one_two three-four xfive_xsix "
			"xseven-eight<rst><c28><c9><ceos>one_two three-four xfive_xsix "
			"xseven-eight<rst><c17><c9><ceos>one_two xthree-four xfive_xsix "
			"xseven-eight<rst><c18><c9><ceos>one_two xthree-four xfive_xsix "
			"xseven-eight<rst><c17><c9><ceos>one_two xthree-four xfive_xsix "
			"xseven-eight<rst><c13><c9><ceos>one_xtwo xthree-four xfive_xsix "
			"xseven-eight<rst><c14><c9><ceos>one_xtwo xthree-four xfive_xsix "
			"xseven-eight<rst><c13><c9><ceos>one_xtwo xthree-four xfive_xsix "
			"xseven-eight<rst><c9><c9><ceos>xone_xtwo xthree-four xfive_xsix "
			"xseven-eight<rst><c10><c9><ceos>xone_xtwo xthree-four xfive_xsix "
			"xseven-eight<rst><c54>\r\n"
			"xone_xtwo xthree-four xfive_xsix xseven-eight\r\n",
			"one_two three-four five_six seven-eight\n",
			command = ReplxxTests._cSample_ + " q1 'w \t_'"
		)
	def test_no_color( self_ ):
		self_.check_scenario(
			"<up> X<cr><c-d>",
			"<c9><ceos>color_black color_red color_green color_brown color_blue "
			"color_magenta color_cyan color_lightgray color_gray color_brightred "
			"color_brightgreen color_yellow color_brightblue color_brightmagenta "
			"color_brightcyan color_white<c70> X<u2><c9><ceos>color_black color_red "
			"color_green color_brown color_blue color_magenta color_cyan color_lightgray "
			"color_gray color_brightred color_brightgreen color_yellow color_brightblue "
			"color_brightmagenta color_brightcyan color_white X<c72>\r\n"
			"color_black color_red color_green color_brown color_blue color_magenta "
			"color_cyan color_lightgray color_gray color_brightred color_brightgreen "
			"color_yellow color_brightblue color_brightmagenta color_brightcyan "
			"color_white X\r\n",
			"color_black color_red color_green color_brown color_blue color_magenta color_cyan color_lightgray"
			" color_gray color_brightred color_brightgreen color_yellow color_brightblue color_brightmagenta color_brightcyan color_white\n",
			command = ReplxxTests._cSample_ + " q1 m1"
		)
	def test_backspace_long_line_on_small_term( self_ ):
		self_.check_scenario(
			"<cr><cr><cr><up><backspace><backspace><backspace><backspace><backspace><backspace><backspace><backspace><cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c14><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c13><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c12><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c11><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c10><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c9><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c8><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c7><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c6><u1><c9><ceos>aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa<rst><c6>\r\n"
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
			dimensions = ( 10, 40 )
		)
		self_.check_scenario(
			"<cr><cr><cr><up><backspace><backspace><backspace><backspace><backspace><backspace><cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos>a qu ite lo ng li ne of sh ort wo rds wi "
			"ll te st cu rs or mo ve me nt<rst><c39><u1><c9><ceos>a qu ite lo "
			"ng li ne of sh ort wo rds wi ll te st cu rs or mo ve me "
			"n<rst><c38><u1><c9><ceos>a qu ite lo ng li ne of sh ort wo rds wi "
			"ll te st cu rs or mo ve me <rst><c37><u1><c9><ceos>a qu ite lo ng "
			"li ne of sh ort wo rds wi ll te st cu rs or mo ve "
			"me<rst><c36><u1><c9><ceos>a qu ite lo ng li ne of sh ort wo rds "
			"wi ll te st cu rs or mo ve m<rst><c35><u1><c9><ceos>a qu ite lo "
			"ng li ne of sh ort wo rds wi ll te st cu rs or mo ve "
			"<rst><c34><u1><c9><ceos>a qu ite lo ng li ne of sh ort wo rds wi "
			"ll te st cu rs or mo ve<rst><c33><u1><c9><ceos>a qu ite lo ng li "
			"ne of sh ort wo rds wi ll te st cu rs or mo ve<rst><c33>\r\n"
			"a qu ite lo ng li ne of sh ort wo rds wi ll te st cu rs or mo ve\r\n",
			"a qu ite lo ng li ne of sh ort wo rds wi ll te st cu rs or mo ve me nt\n",
			dimensions = ( 10, 40 )
		)
	def test_reverse_history_search_on_max_match( self_ ):
		self_.check_scenario(
			"<up><c-r><cr><c-d>",
			"<c9><ceos>aaaaaaaaaaaaaaaaaaaaa<rst><c30><c1><ceos><c1><ceos>(reverse-i-search)`': "
			"aaaaaaaaaaaaaaaaaaaaa<c44><c1><ceos><brightgreen>replxx<rst>> "
			"aaaaaaaaaaaaaaaaaaaaa<c30><c9><ceos>aaaaaaaaaaaaaaaaaaaaa<rst><c30>\r\n"
			"aaaaaaaaaaaaaaaaaaaaa\r\n",
			"aaaaaaaaaaaaaaaaaaaaa\n"
		)
	def test_no_terminal( self_ ):
		res = subprocess.run( [ ReplxxTests._cSample_, "q1" ], input = b"replxx FTW!\n", stdout = subprocess.PIPE, stderr = subprocess.PIPE )
		self_.assertSequenceEqual( res.stdout, b"starting...\nreplxx FTW!\n\nExiting Replxx\n" )
	def test_async_print( self_ ):
		self_.check_scenario(
			[ "a", "b", "c", "d", "e", "f<cr><c-d>" ],
			"<c1><ceos>0\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><c9><c9><ceos>a<rst><c10><c9><ceos>ab<rst><c11><c1><ceos>1\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos>ab<rst><c11><c9><ceos>abc<rst><c12><c9><ceos>abcd<rst><c13><c1><ceos>2\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos>abcd<rst><c13><c9><ceos>abcde<rst><c14><c9><ceos>abcdef<rst><c15><c9><ceos>abcdef<rst><c15>\r\n"
			"abcdef\r\n",
			command = ReplxxTests._cxxSample_ + " a",
			pause = 0.5
		)
		self_.check_scenario(
			[ "<up>", "a", "b", "c", "d", "e", "f<cr><c-d>" ],
			"<c1><ceos>0\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9><c9><ceos>a very long line of "
			"user input, wider then current terminal, the line is wrapped: "
			"<rst><c11><u2><c9><ceos>a very long line of user input, wider then current "
			"terminal, the line is wrapped: a<rst><c12><u2><c1><ceos>1\r\n"
			"<brightgreen>replxx<rst>> \r\n"
			"\r\n"
			"<u2><c9><ceos>a very long line of user input, wider then current terminal, "
			"the line is wrapped: a<rst><c12><u2><c9><ceos>a very long line of user "
			"input, wider then current terminal, the line is wrapped: "
			"ab<rst><c13><u2><c9><ceos>a very long line of user input, wider then current "
			"terminal, the line is wrapped: abc<rst><c14><u2><c1><ceos>2\r\n"
			"<brightgreen>replxx<rst>> \r\n"
			"\r\n"
			"<u2><c9><ceos>a very long line of user input, wider then current terminal, "
			"the line is wrapped: abc<rst><c14><u2><c9><ceos>a very long line of user "
			"input, wider then current terminal, the line is wrapped: "
			"abcd<rst><c15><u2><c9><ceos>a very long line of user input, wider then "
			"current terminal, the line is wrapped: abcde<rst><c16><u2><c1><ceos>3\r\n"
			"<brightgreen>replxx<rst>> \r\n"
			"\r\n"
			"<u2><c9><ceos>a very long line of user input, wider then current terminal, "
			"the line is wrapped: abcde<rst><c16><u2><c9><ceos>a very long line of user "
			"input, wider then current terminal, the line is wrapped: "
			"abcdef<rst><c17><u2><c9><ceos>a very long line of user input, wider then "
			"current terminal, the line is wrapped: abcdef<rst><c17>\r\n"
			"a very long line of user input, wider then current terminal, the line is "
			"wrapped: abcdef\r\n",
			"a very long line of user input, wider then current terminal, the line is wrapped: \n",
			command = ReplxxTests._cxxSample_ + " a",
			dimensions = ( 10, 40 ),
			pause = 0.5
		)

def parseArgs( self, func, argv ):
	global verbosity
	res = func( self, argv )
	verbosity = self.verbosity
	return res

if __name__ == "__main__":
	pa = unittest.TestProgram.parseArgs
	unittest.TestProgram.parseArgs = lambda self, argv: parseArgs( self, pa, argv )
	unittest.main()

