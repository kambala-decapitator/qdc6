#include <QColor>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>

#if SVG_ENABLED
#include <QPainter>
#include <QSvgGenerator>
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

static constexpr auto PaletteSize = 256;
static constexpr auto PaletteComponents = 3;

#if SVG_ENABLED
static constexpr auto svgFormat = "svg";
#endif

struct Dc6Header
{
	uint32_t alwaysSix;
	uint32_t alwaysOne;
	uint32_t alwaysZero;
	uint32_t terminator;
	uint32_t directions;
	uint32_t framesPerDirection;
};

struct Dc6FrameHeader
{
	uint32_t isFlipped;
	uint32_t width;
	uint32_t height;
	uint32_t offsetX;
	uint32_t offsetY;
	uint32_t alwaysZero;
	uint32_t nextFrameIndex;
	uint32_t length;
};


using Options = std::vector<std::string>;

QDebug operator<<(QDebug d, const Options& opts)
{
	Q_ASSERT(!opts.empty());
	d.nospace() << "  " << opts.front().c_str();
	std::for_each(std::next(opts.cbegin()), opts.cend(), [&](const Options::value_type& opt) {
		d << ", " << opt.c_str();
	});
	return d.nospace();
}

bool containsOption(const Options& opts, const char* s)
{
	return std::find(opts.cbegin(), opts.cend(), s) != opts.cend();
}


void printSupportedFormats()
{
	qDebug() << "Supported image output formats:\n" << QImageWriter::supportedImageFormats()
#if SVG_ENABLED
			 << "and" << svgFormat
#endif
				;
}

QString convertedPath(const char* path)
{
	return QString::fromLocal8Bit(path);
}

#if SVG_ENABLED
void saveSvg(const QString& fileName, const QImage& image)
{
	const auto size = image.size();

	QSvgGenerator svg;
	svg.setFileName(fileName);
	svg.setSize(size);
	svg.setViewBox(QRect{{}, size});
	svg.setTitle(QFileInfo{fileName}.completeBaseName());

	QPainter painter;
	if (!painter.begin(&svg)) {
		qCritical() << "can't save svg to" << fileName;
		return;
	}

	const auto pixels = reinterpret_cast<const QRgb*>(image.bits());
	for (int x = 0; x < size.width(); ++x) {
		for (int y = 0; y < size.height(); ++y) {
			painter.setPen(QColor::fromRgba(pixels[y * size.width() + x]));
			painter.drawPoint(x, y);
		}
	}
}
#endif

int main(int argc, char* argv[])
{
	const Options paletteOpts{"-p", "--palette"};
	const Options formatOpts{"-f", "--format"};
	const Options qualityOpts{"-q", "--quality"};
	const Options transparentColorOpts{"-t", "--transparent-color"};
	const Options outDirOpts{"-o", "--out-dir"};
	const Options separateDirOpts{"-d", "--separate-dir"};
	const Options verboseOpts{"-v", "--verbose"};
	const Options treatArgsAsPositionalsOpt{"--"};
	const Options supportedFormatsOpts{"-l", "--list-supported-formats"};
	const Options helpOpts{"-h", "--help"};

	const auto defaultFormat = "png";
	const auto minQuality = 0;
	const auto maxQuality = 100;
	const auto defaultTransparentColor = qRgba(0, 0, 0, 0);
	auto printHelp = [&] {
		const auto treatArgsAsPositionalsOptHelp = QString{"[%1]"}.arg(treatArgsAsPositionalsOpt.front().c_str()).toLatin1();
		const auto defaultTransparentColorStr = QColor{defaultTransparentColor}.name().toLatin1();

		qDebug() << "Usage:" << argv[0] << "[options]" << treatArgsAsPositionalsOptHelp.constData() << "[directory or dc6 path...]\n\nOptions:";
		qDebug() << paletteOpts << " <file>\t\tPalette file to use, defaults to the embedded one";
		qDebug() << formatOpts << " <format>\t\tOutput image format, defaults to " << defaultFormat;
		qDebug() << qualityOpts << " <integer>\tOutput image quality in range " << minQuality << '-' << maxQuality << " inclusive"
#if SVG_ENABLED
				 << ", doesn't apply to " << svgFormat
#endif
					;
		qDebug() << transparentColorOpts << " <str>\tColor to use as transparent, defaults to " << defaultTransparentColorStr.constData() << ", see QColor::setNamedColor() for full list of supported formats";
		qDebug() << outDirOpts << " <directory>\tWhere to save output files, defaults to input file's directory";
		qDebug() << separateDirOpts << "\t\tSave multiframe images in a directory named after the input file";
		qDebug() << verboseOpts << "\t\t\tVerbose output";
		qDebug();
		qDebug() << supportedFormatsOpts << "\tPrint supported image formats";
		qDebug() << helpOpts << "\t\t\tPrint this message\n";
		printSupportedFormats();
	};

	QStringList dc6Paths;
	QString palettePath;
	auto imageFormat = defaultFormat;
	int imageQuality = -1;
	auto transparentColor = defaultTransparentColor;
	QString outDirPath;
	auto useSeparateDir = false;
	auto treatArgsAsPositionals = false;
	auto verboseOutput = false;
	auto showSupportedFormats = false;
	auto showHelp = false;

	using Argument = const char*;
	for (int i = 1; i < argc; ++i) {
		if (treatArgsAsPositionals) {
			dc6Paths << convertedPath(argv[i]);
			continue;
		}

		auto processNextArg = [&](const std::function<void(Argument)>& processor) {
			if (i + 1 < argc)
				processor(argv[++i]);
		};

		if (containsOption(helpOpts, argv[i]))
			showHelp = true;
		else if (containsOption(supportedFormatsOpts, argv[i]))
			showSupportedFormats = true;
		else if (containsOption(paletteOpts, argv[i]))
			processNextArg([&](Argument arg) {
				palettePath = convertedPath(arg);
			});
		else if (containsOption(formatOpts, argv[i]))
			processNextArg([&](Argument arg) {
				imageFormat = arg;
			});
		else if (containsOption(qualityOpts, argv[i]))
			processNextArg([&](Argument arg) {
				try {
					imageQuality = std::stoi(arg);
					if (imageQuality < minQuality || imageQuality > maxQuality) {
						imageQuality = -1;
						qWarning() << "image quality exceeds valid range, default setting will be used";
					}
				}
				catch (const std::exception& e) {
					qWarning() << "couldn't convert image quality to number, default setting will be used:" << e.what();
				}
			});
		else if (containsOption(transparentColorOpts, argv[i]))
			processNextArg([&](Argument arg) {
				const QColor color{arg};
				if (color.isValid())
					transparentColor = color.rgba();
			});
		else if (containsOption(outDirOpts, argv[i]))
			processNextArg([&](Argument arg) {
				outDirPath = convertedPath(arg);
			});
		else if (containsOption(separateDirOpts, argv[i]))
			useSeparateDir = true;
		else if (containsOption(verboseOpts, argv[i]))
			verboseOutput = true;
		else if (containsOption(treatArgsAsPositionalsOpt, argv[i]))
			treatArgsAsPositionals = true;
		else
			dc6Paths << convertedPath(argv[i]);
	}

	if (showHelp || argc == 1) {
		printHelp();
		return 0;
	}
	if (showSupportedFormats) {
		printSupportedFormats();
		return 0;
	}

	if (dc6Paths.isEmpty()) {
		qWarning() << "no input files specified";
		return 1;
	}
	if (!outDirPath.isEmpty() && !QDir{outDirPath}.mkpath(".")) {
		qCritical() << "unable to create output directory at" << outDirPath;
		return 1;
	}

	for (int i = 0; i < dc6Paths.size(); ++i) {
		const QDir dir{dc6Paths[i]};
		if (!dir.exists())
			continue;
		dc6Paths.removeAt(i--);

		const auto dc6InDir = dir.entryList(QStringList{QLatin1String{"*.dc6"}}, QDir::Files | QDir::Readable);
		if (verboseOutput)
			qDebug() << "files in dir" << dir << ':' << dc6InDir;
		for (const auto& dc6Filename : dc6InDir)
			dc6Paths += dir.absoluteFilePath(dc6Filename);
	}

	if (palettePath.isEmpty()) {
		palettePath = QLatin1String{":/units_pal.dat"};
		if (verboseOutput)
			qDebug() << "using embedded palette";
	}
	QFile paletteFile{palettePath};
	if (!paletteFile.open(QFile::ReadOnly)) {
		qCritical() << "error opening palette file:" << paletteFile.errorString();
		return 1;
	}
	const auto paletteFileSize = QFileInfo{paletteFile}.size();
	if (paletteFileSize != PaletteSize * PaletteComponents) {
		qCritical() << "palette file has wrong size:" << paletteFileSize;
		return 1;
	}

	std::vector<QRgb> colorPalette;
	colorPalette.reserve(PaletteSize);

	QDataStream ds{&paletteFile};
	while (!ds.atEnd()) {
		std::array<uint8_t, PaletteComponents> bgr;
		ds >> bgr[0] >> bgr[1] >> bgr[2];
		colorPalette.push_back(qRgb(bgr[2], bgr[1], bgr[0]));
	}
	paletteFile.close();

	for (const auto& dc6Path : dc6Paths) {
		if (verboseOutput)
			qDebug() << "processing file" << dc6Path;

		QFile f{dc6Path};
		if (!f.open(QFile::ReadOnly)) {
			qCritical() << "error opening dc6 file:" << dc6Path << '\n' << f.errorString();
			continue;
		}
		ds.setDevice(&f);
		ds.setByteOrder(QDataStream::LittleEndian);

		Dc6Header header;
		ds >> header.alwaysSix;
		ds >> header.alwaysOne;
		ds >> header.alwaysZero;
		if (header.alwaysSix != 6 || header.alwaysOne != 1 || header.alwaysZero != 0) {
			qCritical() << "invalid header in file" << dc6Path;
			continue;
		}

		ds.skipRawData(sizeof header.terminator);
		ds >> header.directions;
		ds >> header.framesPerDirection;
		const auto framesTotal = header.directions * header.framesPerDirection;
		if (verboseOutput)
			qDebug() << header.directions << "direction(s) with" << header.framesPerDirection << "frame(s) =" << framesTotal << "frames total";

		std::vector<uint32_t> frameIndexes;
		frameIndexes.resize(framesTotal);
		for (std::size_t i = 0; i < framesTotal; ++i)
			ds >> frameIndexes[i];

		const auto dc6BaseName = QFileInfo{f}.completeBaseName();
		auto outImageBasePath = dc6BaseName;
		if (!outDirPath.isEmpty())
			outImageBasePath.prepend(outDirPath + '/');
		if (framesTotal > 1) {
			if (useSeparateDir) {
				QDir{outImageBasePath}.mkpath(".");
				outImageBasePath += '/';
			}
			else
				outImageBasePath += '_';
		}

		std::size_t j = 0;
		for (auto index : frameIndexes) {
			f.seek(index);

			if (verboseOutput)
				qDebug() << "frame index" << j << ", offset" << index;

			Dc6FrameHeader frameHeader;
			ds >> frameHeader.isFlipped;
			ds >> frameHeader.width;
			ds >> frameHeader.height;
			ds.skipRawData(sizeof frameHeader.offsetX);
			ds.skipRawData(sizeof frameHeader.offsetY);
			ds.skipRawData(sizeof frameHeader.alwaysZero);
			ds >> frameHeader.nextFrameIndex;
			ds >> frameHeader.length;

			if (verboseOutput)
				qDebug() << "width =" << frameHeader.width << ", height =" << frameHeader.height << ", length =" << frameHeader.length;

			std::vector<QRgb> pixels(frameHeader.width * frameHeader.height, transparentColor);
			std::size_t pixI = 0;
			for (std::size_t i = 0; i < frameHeader.length; ++i) {
				uint8_t pixel;
				ds >> pixel;
				const auto writeTransparent = (pixel & 0b10000000) != 0;
				const auto pixelsToAdd = pixel & 0b01111111;
				if (writeTransparent) {
					if (pixelsToAdd > 0)
						pixI += pixelsToAdd;
					else
						pixI = (pixI / frameHeader.width + 1) * frameHeader.width;
				}
				else {
					i += pixelsToAdd;

					for (std::size_t k = 0; k < pixelsToAdd; ++k, ++pixI) {
						ds >> pixel;
						pixels[pixI] = colorPalette[pixel];
					}
					// if filled to the end of scan line, move one pixel back to stay on the current line because next instruction is "advance to next line"
					if (pixI % frameHeader.width == 0)
						--pixI;
				}
			}

			QImage image(reinterpret_cast<const uchar*>(pixels.data()), frameHeader.width, frameHeader.height, QImage::Format_ARGB32_Premultiplied);
			if (!frameHeader.isFlipped)
				image = image.mirrored();

			auto outImageBaseName = outImageBasePath;
			if (framesTotal > 1)
				outImageBaseName += QString::number(j);
			outImageBaseName += '.';
			const auto outFileName = outImageBaseName + imageFormat;

			auto verbosePrintOutFileName = [&](const QString& fileName) {
				if (verboseOutput)
					qDebug() << "save image to" << fileName;
			};
#if SVG_ENABLED
			if (outFileName.endsWith(QLatin1String{svgFormat}, Qt::CaseInsensitive)) {
				verbosePrintOutFileName(outFileName);
				saveSvg(outFileName, image);
			}
			else
#endif
			{
				QImageWriter imageWriter{outFileName};
				if (!imageWriter.canWrite()) {
					imageWriter.setFileName(outImageBaseName + defaultFormat);
					qWarning() << "can't save using the specified format, falling back to" << defaultFormat;
				}
				verbosePrintOutFileName(imageWriter.fileName());
				if (imageQuality > -1)
					imageWriter.setQuality(imageQuality);
				if (!imageWriter.write(image))
					qCritical() << "error saving output image:" << imageWriter.errorString();
			}

			++j;
			if (verboseOutput)
				qDebug() << "-----";
		}
	}

	if (verboseOutput)
		qDebug() << "all images processed";
	return 0;
}
