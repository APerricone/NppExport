#pragma once

#include "Exporter.h"

#define CF_HTML			TEXT("HTML Format")

struct ExportData;
class Exporter;

class HTMLExporter :
	public Exporter
{
public:
	HTMLExporter(void);
	~HTMLExporter(void);
	bool exportData(ExportData * ed);
	TCHAR * getClipboardType() { return CF_HTML; }

private:
	///! inline style information
	char* styles[NRSTYLES];

	///! fills "styles"
	void PrepareStyles(CurrentScintillaData* csd, bool isClipboard);

	///! write All HTML for the clipboard
	///! \param csd the info to write
	///! \param dest optional, the destination buffer
	///! \return the size of written data
	size_t WriteHTMLClipboard(CurrentScintillaData* csd,char* const dest,size_t destSize);

	///! write All HTML for the file
	///! \param csd the info to write
	///! \param dest optional, the destination buffer
	///! \return the size of written data
	size_t WriteHTMLFile(CurrentScintillaData* csd, char* const dest, size_t destSize);
};
