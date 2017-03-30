#include "stdafx.h"
#include "Scintilla.h"
#include "ExportStructs.h"
#include "Exporter.h"
#include "HTMLExporter.h"
#include <stdio.h>

HTMLExporter::HTMLExporter(void) {
	setClipboardID(RegisterClipboardFormat(CF_HTML));
	if (getClipboardID() == 0) {
		MessageBox(NULL, TEXT("Unable to register clipboard format HTML!"), TEXT("Error"), MB_OK);
	}

	memset(styles, 0, sizeof(styles));
}

HTMLExporter::~HTMLExporter(void)
{
	for (int i = 0; i < NRSTYLES; i++)
		if (styles[i])
			delete[] styles[i];
}

void HTMLExporter::PrepareStyles(CurrentScintillaData* csd)
{
	const StyleData& defaultStyle = csd->styles[STYLE_DEFAULT];
	char buff[1024];
	for (int i = 0; i < NRSTYLES; i++)
	{
		if (!csd->usedStyles[i]) continue;

		const StyleData& currentStyle = csd->styles[i];
		int len = 0;
		buff[0] = 0;
		if (i== STYLE_DEFAULT || lstrcmpiA(currentStyle.fontString, defaultStyle.fontString))
			len += sprintf_s(buff + len, 1024 - len, "font-family: '%s', monospace; ", currentStyle.fontString);
		if (i == STYLE_DEFAULT || currentStyle.size != defaultStyle.size)
			len += sprintf_s(buff + len, 1024 - len, "font-size: %dpt; ", currentStyle.size);
		if (i == STYLE_DEFAULT || currentStyle.bold != defaultStyle.bold)
		{
			if (currentStyle.bold)
				len += sprintf_s(buff + len, 1024 - len, "font-weight: bold; ");
			else
				len += sprintf_s(buff + len, 1024 - len, "font-weight: normal; ");
		}
		if (i == STYLE_DEFAULT || currentStyle.italic != defaultStyle.italic)
		{
			if (currentStyle.italic)
				len += sprintf_s(buff + len, 1024 - len, "font-style: italic; ");
			else
				len += sprintf_s(buff + len, 1024 - len, "font-style: normal; ");
		}
		if (i == STYLE_DEFAULT || currentStyle.underlined != defaultStyle.underlined)
		{
			if (currentStyle.underlined)
				len += sprintf_s(buff + len, 1024 - len, "font-decoration: underline; ");
			else
				len += sprintf_s(buff + len, 1024 - len, "font-decoration: none; ");
		}
		if (i == STYLE_DEFAULT || currentStyle.fgColor != defaultStyle.fgColor)
		{
			len += sprintf_s(buff + len, 1024 - len, "color: #%02X%02X%02X; ", (currentStyle.fgColor >> 0) & 0xFF, (currentStyle.fgColor >> 8) & 0xFF, (currentStyle.fgColor >> 16) & 0xFF);
		}
		if (i == STYLE_DEFAULT || currentStyle.bgColor != defaultStyle.bgColor)
		{
			len += sprintf_s(buff + len, 1024 - len, "tbackground: #%02X%02X%02X; ", (currentStyle.bgColor >> 0) & 0xFF, (currentStyle.bgColor >> 8) & 0xFF, (currentStyle.bgColor >> 16) & 0xFF);
		}
		if (i == STYLE_DEFAULT)
		{
			len += sprintf_s(buff + len, 1024 - len, "white-space: pre; tab-size: %i; line-height: normal;", csd->tabSize);
		}
		styles[i] = new char[len+1];
		strcpy_s(styles[i], len + 1, buff);
	}
}

size_t HTMLExporter::WriteHTML(CurrentScintillaData* csd, char *const dest, size_t destSize)
{
	size_t len = 0;
	if (dest)
		len += sprintf_s(dest + len, destSize - len, "<html><body style=\"%s\">\r\n", styles[STYLE_DEFAULT]);
	else
		len += 23 + strlen(styles[STYLE_DEFAULT]);
	char lastStyle = -1;
	char *buffer = csd->dataBuffer;
	for (int i = 0; i < csd->nrChars; i++)
	{
		//print new span object if style changes
		if (buffer[i * 2 + 1] != lastStyle) {
			if (lastStyle!=-1)
			{
				if (dest)
					len += sprintf_s(dest + len, destSize - len, "</span>");
				else
					len += 7;
			}
			lastStyle = buffer[i * 2 + 1];
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "<span style=\"%s\">", styles[lastStyle]);
			else
				len += 15 + strlen(styles[lastStyle]);
		}
		//print character, parse special ones
		char currentChar = buffer[(i * 2)];
		switch (currentChar) {
		case '\r':
			if (buffer[(i * 2) + 2] == '\n' || buffer[(i * 2) + 2] == '\r')
				break;
		case '\n':
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "<br>");
			else
				len += 4;
			break;
		case '<':
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "&lt;");
			else
				len += 4;
			break;
		case '>':
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "&gt;");
			else
				len += 4;
			break;
		case '&':
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "&amp;");
			else
				len += 5;
			break;
		case '\t':
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "\t");
			else
				len += 1;
			break;
		default:
			if (currentChar < 0x20)	//ignore control characters
				break;
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "%c", currentChar);
			else
				len += 1;
			break;
		}
	}
	if (lastStyle != -1)
	{
		if (dest)
			len += sprintf_s(dest + len, destSize - len, "</span>");
		else
			len += 7;
	}
	if (dest)
	{
		len += sprintf_s(dest + len, destSize - len, "</body></html>\r\n");
	}
	else
	{
		len += 16;
	}
	return len;
}

bool HTMLExporter::exportData(ExportData * ed)
{
	PrepareStyles(ed->csd);
	size_t totalHTML = WriteHTML(ed->csd, 0, 0);
	char* header = 0;
	size_t headerLen = 0;
	if (ed->isClipboard)
	{
		header = new char[200];
		headerLen = 20;
		for (int i = 0; i < 2; i++)
			headerLen = sprintf_s(header, 200, "Version:0.9\r\n"
				"StartHTML:%i\r\n"
				"EndHTML:%i\r\n"
				"StartFragment:%i\r\n"
				"EndFragment:%i\r\n", (int)headerLen, (int)(headerLen + totalHTML), (int)(headerLen + 6), (int)(headerLen + totalHTML - 9));

	}
	HGLOBAL hHTMLBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, headerLen+ totalHTML+1);
	char * clipbuffer = (char *)GlobalLock(hHTMLBuffer);
	if (header)
		strcpy_s(clipbuffer, headerLen + totalHTML + 1, header);

	WriteHTML(ed->csd, clipbuffer + headerLen, totalHTML + 1);

	GlobalUnlock(hHTMLBuffer);
	ed->hBuffer = hHTMLBuffer;
	ed->bufferSize = (unsigned long)(headerLen + totalHTML + 1);
	return true;
}
