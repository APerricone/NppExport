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

void HTMLExporter::PrepareStyles(CurrentScintillaData* csd, bool isClipboard)
{
	const StyleData& defaultStyle = csd->styles[STYLE_DEFAULT];
	char buff[1024];
	char* space = "\r\n\t";
	if (isClipboard) space = " ";
	for (int i = 0; i < NRSTYLES; i++)
	{
		if (!csd->usedStyles[i]) continue;

		const StyleData& currentStyle = csd->styles[i];
		int len = 0;
		buff[0] = 0;
		if (i== STYLE_DEFAULT || lstrcmpiA(currentStyle.fontString, defaultStyle.fontString))
			len += sprintf_s(buff + len, 1024 - len, "font-family: '%s', monospace;%s", currentStyle.fontString, space);
		if (i == STYLE_DEFAULT || currentStyle.size != defaultStyle.size)
			len += sprintf_s(buff + len, 1024 - len, "font-size: %dpt;%s", currentStyle.size, space);
		if (i == STYLE_DEFAULT || currentStyle.bold != defaultStyle.bold)
		{
			if (currentStyle.bold)
				len += sprintf_s(buff + len, 1024 - len, "font-weight: bold;%s", space);
			else
				len += sprintf_s(buff + len, 1024 - len, "font-weight: normal;%s", space);
		}
		if (i == STYLE_DEFAULT || currentStyle.italic != defaultStyle.italic)
		{
			if (currentStyle.italic)
				len += sprintf_s(buff + len, 1024 - len, "font-style: italic;%s", space);
			else
				len += sprintf_s(buff + len, 1024 - len, "font-style: normal;%s", space);
		}
		if (i == STYLE_DEFAULT || currentStyle.underlined != defaultStyle.underlined)
		{
			if (currentStyle.underlined)
				len += sprintf_s(buff + len, 1024 - len, "font-decoration: underline;%s", space);
			else
				len += sprintf_s(buff + len, 1024 - len, "font-decoration: none;%s", space);
		}
		if (i == STYLE_DEFAULT || currentStyle.fgColor != defaultStyle.fgColor)
		{
			len += sprintf_s(buff + len, 1024 - len, "color: #%02X%02X%02X;%s", (currentStyle.fgColor >> 0) & 0xFF, (currentStyle.fgColor >> 8) & 0xFF, (currentStyle.fgColor >> 16) & 0xFF, space);
		}
		if (i == STYLE_DEFAULT || currentStyle.bgColor != defaultStyle.bgColor)
		{
			len += sprintf_s(buff + len, 1024 - len, "background-color: #%02X%02X%02X;%s", (currentStyle.bgColor >> 0) & 0xFF, (currentStyle.bgColor >> 8) & 0xFF, (currentStyle.bgColor >> 16) & 0xFF, space);
		}
		if (i == STYLE_DEFAULT)
		{
			len += sprintf_s(buff + len, 1024 - len, "white-space: pre;%s", space);
			len += sprintf_s(buff + len, 1024 - len, "tab-size: %i;%s", csd->tabSize, space);
			len += sprintf_s(buff + len, 1024 - len, "line-height: normal;%s", space);
		}
		styles[i] = new char[len+1];
		strcpy_s(styles[i], len + 1, buff);
	}
}

size_t HTMLExporter::WriteHTMLClipboard(CurrentScintillaData* csd, char *const dest, size_t destSize)
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

size_t HTMLExporter::WriteHTMLFile(CurrentScintillaData* csd, char *const dest, size_t destSize)
{
	size_t len = 0;
	if (dest)
		len += sprintf_s(dest + len, destSize - len,
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/1999/REC-html401-19991224/strict.dtd\">\r\n"
			"<html>\r\n"
			"<head>\r\n"
			"<META http-equiv=Content-Type content=\"text/html; charset=%s\">\r\n"
			"<title>Exported from Notepad++</title>\r\n"
			"<style type=\"text/css\">\r\n",
				csd->currentCodePage == SC_CP_UTF8 ? "UTF-8": "windows-1252");
	else
		len += 255 + (csd->currentCodePage == SC_CP_UTF8 ? 5 : 12);
	// write style
	if (dest)
		len += sprintf_s(dest + len, destSize - len, "body\r\n{\r\n\t%s}\r\n",styles[STYLE_DEFAULT]);
	else
		len += 13+strlen(styles[STYLE_DEFAULT]);

	for (int i = 0; i < NRSTYLES; i++)
	{
		if (i==STYLE_DEFAULT || !csd->usedStyles[i]) continue;

		if (dest)
			len += sprintf_s(dest + len, destSize - len, ".sc%i\r\n{\r\n\t%s}\r\n", i,styles[i]);
		else
			len += 13 + (i<10? 0 : 1) + +strlen(styles[i]);
	}

	if (dest)
		len += sprintf_s(dest + len, destSize - len, "</style>\r\n</head>\r\n<body>\r\n");
	else
		len += 27;

	// write body
	char lastStyle = -1;
	char *buffer = csd->dataBuffer;
	int nTab = 0;
	bool justOpened = false;
	for (int i = 0; i < csd->nrChars; i++)
	{
		//print new span object if style changes
		if (buffer[i * 2 + 1] != lastStyle) {
			if (lastStyle != -1)
			{
				if (dest)
					len += sprintf_s(dest + len, destSize - len, "</span>");
				else
					len += 7;
			}
			lastStyle = buffer[i * 2 + 1];
			justOpened = buffer[(i * 2) + 2] == '\t';
			if (dest)
			{
				if (justOpened)
				{
					len += sprintf_s(dest + len, destSize - len, "<span class=\"sc%i\" ", lastStyle);
				}
				else
				{
					len += sprintf_s(dest + len, destSize - len, "<span class=\"sc%i\">", lastStyle);
				}
			}
			else
				len += 18 + (lastStyle<10 ? 0 : 1);
		}
		//print character, parse special ones
		char currentChar = buffer[(i * 2)];
		switch (currentChar) {
		case '\r':
			if (buffer[(i * 2) + 2] == '\n' || buffer[(i * 2) + 2] == '\r')
				break;
		case '\n':
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "\r\n");
			else
				len += 2;
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
			nTab = csd->tabSize;
			while (buffer[(i * 2) + 2] == '\t')
			{
				nTab += csd->tabSize;
				i++;
			}
			if (!justOpened)
			{
				if (dest)
					len += sprintf_s(dest + len, destSize - len, "</span><span class=\"sc%i\" ", lastStyle);
				else
					len += 25 + (lastStyle<10 ? 0 : 1);
			}
			if (dest)
				len += sprintf_s(dest + len, destSize - len, "style=\"margin-left: %iex;\">", nTab);
			else
				len += 26 + (lastStyle<10? 0 : 1) + (nTab<10? 0 : 1) + (nTab<100 ? 0 : 1) + (nTab<1000 ? 0 : 1);
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
		len += sprintf_s(dest + len, destSize - len, "</body>\r\n</html>\r\n");
	else
		len += 18;
	return len;
}

bool HTMLExporter::exportData(ExportData * ed)
{
	PrepareStyles(ed->csd, ed->isClipboard);
	char* header = 0;
	size_t headerLen = 0;
	size_t totalHTML = 0;
	if (ed->isClipboard)
	{
		totalHTML = WriteHTMLClipboard(ed->csd, 0, 0);
		header = new char[200];
		headerLen = 20;
		for (int i = 0; i < 2; i++)
			headerLen = sprintf_s(header, 200, "Version:0.9\r\n"
				"StartHTML:%i\r\n"
				"EndHTML:%i\r\n"
				"StartFragment:%i\r\n"
				"EndFragment:%i\r\n", (int)headerLen, (int)(headerLen + totalHTML), (int)(headerLen + 6), (int)(headerLen + totalHTML - 9));

	} else
		totalHTML = WriteHTMLFile(ed->csd, 0, 0);
	HGLOBAL hHTMLBuffer = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, headerLen+ totalHTML+1);
	char * clipbuffer = (char *)GlobalLock(hHTMLBuffer);
	if (header)
		strcpy_s(clipbuffer, headerLen + totalHTML + 1, header);

	if (ed->isClipboard)
		WriteHTMLClipboard(ed->csd, clipbuffer + headerLen, totalHTML + 1);
	else
		WriteHTMLFile(ed->csd, clipbuffer + headerLen, totalHTML + 1);

	GlobalUnlock(hHTMLBuffer);
	ed->hBuffer = hHTMLBuffer;
	ed->bufferSize = (unsigned long)(headerLen + totalHTML);
	return true;
}
