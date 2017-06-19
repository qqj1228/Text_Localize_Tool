# Text_Localize_Tool

## 一个用于汉化翻译的辅助工具

这个小工具是我在2010年左右汉化psp文字冒险游戏《流行之神2》第0章和第1章的时候做的。目的是想要提高翻译效率。

本工具是在参考了“Agemo's Script Editor 文本对照编辑器”的基础上编写而成。

“Agemo's Script Editor 文本对照编辑器”原先是用VB写的，且代码没有很好优化，故我用vc重写并加入大量新功能。

开发环境为VS2008，WTL8.1，使用IE COM控件技术，GBK编码。

开发该工具的年代古老，现在本人已不做汉化相关事情，故不再更新维护。

使用说明：

1. 现仅支持“地址,长度,文本”的导出文本格式，且编码格式必须为unicode(UTF-16 LE带BOM)，其他格式的文本文件打开会报错。

2. 控制符仅支持用“{}”来表示。控制符高亮为蓝色，默认无法编辑控制符，不过可以在选项里修改成允许修改控制符。

3. 本工具可以精确对比翻译文本和对照文本。显示出两者不同之处，以红色+下划线显示。若对照译文和中文译文无区别时，对照译文以“+-+-+-+-+-+”显示。若不勾选“中文”列完整高亮显示差异文字选项的话，则“中文”列只会把相对于对照文本多出来的字以红色+下划线显示，其余不同之处使用普通文字颜色。

4. 文本越界会有提示。若出现文本越界的情况，该越界的整个条目会以黄色背景高亮显示，并且该行的“NO”列也会以黄色背景高亮显示。提示框中同时会显示第几行超长多少字节，提示信息框背景色同时变为黄色。双击该条提示信息的话，程序会自动跳到该行。当把当前页内的所有文本越界情况都修正后，提示框背景会还原为默认色。

5. 文本越界计算规则：

    1. 若使用外部控制符长度定义文件（Ctrl_Len.txt），则控制符所占字节数由该文件定义。其格式为：控制符=所占字节数（10进制）其数量控制在：单个控制符长度必须小于14个字符，整个控制符数量应小于650个。
   
    2. 若不使用外部控制符长度定义文件或者存在外部控制符长度文件中未定义过的控制符，则其所占字节数由以下规则定义：
   
        1. 工具会扫描每个“{}”表示的控制符。如果找到“,”（英文半角逗号）的话，就会读取紧跟其后的10进制数字，该数字即表示该控制符所占的字节数。
     
        2. 若控制符内有多个“,”，则以第一个找到的“,”为准。
     
        3. 若没有找到“,”，则程序默认该控制符占8个字节。
     
    3. 若文本中有半角字符，则每个半角字符占1个字节。
   
    4. 其余全角字符所占字节数以“文本中每个全角字符所占字节数”选项值控制。
   
    5. 使用外部控制符长度定义文件的话会降低打开文件速度，降低程度由该文件大小和原文本所含控制符数量决定。

6. 预览界面上方的导航条滑块用来指示当前页。点击该导航条可以手动刷新当前文件。当文本文件的总行数大于选项中“每页显示行数”的数值时，会分成若干页显示。点击滑块的左/右侧，会进入上/下一页，或者直接拖动滑块到想要的页数。

7. 当预览界面内的文本行数达到1万行左右这个数量时预览显示速度会明显变慢，请合理设置每页的显示行数。选项里有两项和显示速度有关，即：“每页显示行数”和“表格生成粒度”。前一项的数值必须能够整除后一项的数值，若不能整除的话程序会自动把前一项更改为合适的数值。

8. 当输入焦点在编辑框内时，同时按“Ctrl+<”或“Ctrl+>”可以直接进入上/下一句，输入焦点不在编辑框内时该快捷键无效。

9. 程序会自动保存，保存时会忽略回车换行符。

10. 可以设置输入的标点、字母等是否要自动从半角转换成全角，控制符内不会转换。

11. 工具具有查找替换功能：“查找当前页”会在预览框内的当前显示页内查找关键词，“替换译文”会在当前中文译文范围内查找/替换关键词，不局限于当前页，搜索到的关键词会在编辑框内选中或是被替换，“全部替换”则是直接替换关键词，不在编辑框内显示。

12. 工具安装目录下的“Ctrl_Effect.txt”文件为特殊效果配置文件。可以把文本中某些字、词、控制符等，在预览界面内替换成想要达到的特殊效果字符。该效果只有在“隐藏控制符”按钮处于按下状态时，才会显示。弹起状态时，呈现的是原始字符。用法很简单，每行一个替换效果：“原始字符=特殊效果”。其中“特殊效果”必须是html代码。注意：如果想要替换的字符是半角空格时，在“=”之前要用“&ampnbsp;”代替真实的半角空格字符。

13. 工具具有重复文本自动翻译功能：该功能会搜索当前行及之后的日文原文是否有和当前行之前的日文原文相同的条目（普通文本和控制符都相同才算相同）。若有的话，会把对应的重复日文原文自动翻译好。注意：

    1. 重复文本自动翻译的内容以第一次找到的相同日文原文所对应的译文为准，若该译文还未翻译，则继续向下寻找，直到找到一个翻译过的译文。若找不到翻译过的译文，则不翻译。

    2. 若当前行及以后的日文原文已经翻译过，会自动跳转到该行并提示是否要覆盖。

14. 在宽度选项中“中文”列宽度和“对照”列宽度选项的数值若设为0的话，则预览界面内不显示相应列。

15. 在按下“跳转至...”按钮后打开的对话框内，双击空行列表可以直接跳转至该行。或者使用“下一行”/“上一行”按钮来分别跳转至下一个空行/上一个空行。或者在编辑框内填入想要跳转的行数，按下“确定”按钮，会直接跳转至该行。

工具目录结构：
```
\				工具安装目录
|
+-------jp-text			日文文本存放文件夹
|				存放导出的文本文本文件
|
+-------cn-text			译文文本存放文件夹
|				存放翻译好的中文译文，文件名同对应的日文原文文件
|
+-------cn-compare	        对照译文存放文件夹
|				存放对照或润色的文本，文件名同对应的日文原文文件
|
+-------temp			缓存文件夹（仅在获取系统缓存文件夹失败时需要）
|	|
|	+--------temp.html	预览缓存文件（仅在获取系统缓存文件夹失败时创建）
|
+-------Text_Localize_Tool.exe	工具主程序
|
+-------Config.ini		工具配置文件
|
+-------Ctrl_Effect.txt		特殊效果配置文件
|
+-------Ctrl_Len.txt		控制符长度文件
```
已知bug：

1. 如果句首就是控制符的话，首次在句首插入中文（即打开输入法输入字符），插入的字符会被误认为是控制符。解决方法：先在关闭输入法的状态下输入一个英文字符，然后再打开输入法输入中文即可正常显示。输入完后再删除开头输入的英文字符。

2. 在两个控制符之间（即在"}"和"{"之间）输入中文字符（即打开输入法输入字符），输入的字符会被误认为是控制符。解决方法：先在关闭输入法的状态下输入一个英文字符，然后再打开输入法输入中文即可正常显示。输入完后再删除开头输入的英文字符。

3. 在个别机器上关闭本工具后程序会崩溃（原因不明-，-）。
