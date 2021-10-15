#include <QColor>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

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
	d.nospace() << opts.front().c_str();
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
	qDebug() << "Supported image output formats:\n" << QImageWriter::supportedImageFormats();
}

int main(int argc, char* argv[])
{
	const Options paletteOpts{"-p", "--palette"};
	const Options formatOpts{"-f", "--format"};
	const Options qualityOpts{"-q", "--quality"};
	const Options supportedFormatsOpts{"-l", "--list-supported-formats"};
	const Options helpOpts{"-h", "--help"};
	
	const auto defaultFormat = "png";
	auto printHelp = [&] {
		qDebug() << "Usage:" << argv[0] << "[options] [dc6 paths...]\n\nOptions:";
		qDebug() << paletteOpts << " <file>\tPalette file to use";
		qDebug() << formatOpts << " <format>\tOutput image format, defaults to" << defaultFormat;
		qDebug() << qualityOpts << " <integer>\tOutput image quality in range 0-100";
		qDebug() << supportedFormatsOpts << "\tPrint supported image formats";
		qDebug() << helpOpts << "\t\tPrint this message\n";
		printSupportedFormats();
	};

	QStringList dc6Paths;
	QString palettePath;
	auto imageFormat = defaultFormat;
	int imageQuality = -1;
	auto showSupportedFormats = false;
	auto showHelp = false;

	for (int i = 1; i < argc; ++i) {
		const auto hasNextArg = i + 1 < argc;
		if (containsOption(helpOpts, argv[i]))
			showHelp = true;
		else if (containsOption(supportedFormatsOpts, argv[i]))
			showSupportedFormats = true;
		else if (containsOption(paletteOpts, argv[i]) && hasNextArg)
			palettePath = QString::fromLocal8Bit(argv[++i]);
		else if (containsOption(formatOpts, argv[i]) && hasNextArg)
			imageFormat = argv[++i];
		else if (containsOption(qualityOpts, argv[i]) && hasNextArg) {
			try {
				imageQuality = std::stoi(argv[++i]);
				if (imageQuality < 0 || imageQuality > 100) {
					imageQuality = -1;
					qWarning() << "image quality exceeds valid range, default setting will be used";
				}
			}
			catch (const std::exception& e) {
				qWarning() << "couldn't convert image quality to number, default setting will be used:" << e.what();
			}
		}
		else
			dc6Paths << QString::fromLocal8Bit(argv[i]);
	}

	if (showHelp || argc == 1) {
		printHelp();
		return 0;
	}
	if (showSupportedFormats) {
		printSupportedFormats();
		return 0;
	}

	if (palettePath.isEmpty()) {
		qCritical() << "palette file is required";
		return 1;
	}
	if (dc6Paths.isEmpty()) {
		qWarning() << "no input files specified";
		return 1;
	}

	QFile paletteFile{palettePath};
	if (!paletteFile.open(QFile::ReadOnly)) {
		qCritical() << "error opening palette file:" << paletteFile.errorString();
		return 1;
	}

	std::vector<QRgb> colorPalette;
	colorPalette.reserve(256);

	QDataStream ds{&paletteFile};
	while (!ds.atEnd()) {
		uint8_t r, g, b;
		ds >> b >> g >> r;
		colorPalette.push_back(qRgb(r, g, b));
	}
	Q_ASSERT(colorPalette.size() == 256);
	paletteFile.close();

	for (const auto& dc6Path : dc6Paths) {
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

		ds >> header.terminator;
		ds >> header.directions;
		ds >> header.framesPerDirection;
		const auto framesTotal = header.directions * header.framesPerDirection;
		qDebug() << header.directions << "direction(s) with" << header.framesPerDirection << "frame(s) =" << framesTotal << "frames total";

		std::vector<uint32_t> frameIndexes;
		frameIndexes.resize(framesTotal);
		for (std::size_t i = 0; i < framesTotal; ++i)
			ds >> frameIndexes[i];

		const auto dc6BaseName = QFileInfo{f}.completeBaseName();
		std::size_t j = 0;
		for (auto index : frameIndexes) {
			f.seek(index);
			qDebug() << j << "frame, index =" << index;

			Dc6FrameHeader frameHeader;
			ds >> frameHeader.isFlipped;
			ds >> frameHeader.width;
			ds >> frameHeader.height;
			ds >> frameHeader.offsetX;
			ds >> frameHeader.offsetY;
			ds >> frameHeader.alwaysZero;
			ds >> frameHeader.nextFrameIndex;
			ds >> frameHeader.length;
			qDebug() << "w =" << frameHeader.width << "h =" << frameHeader.height << "l =" << frameHeader.length;

			std::vector<QRgb> pixels(frameHeader.width * frameHeader.height, qRgba(0, 0, 0, 0));
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

			const auto outImageBaseName = QString("%1_%2.").arg(dc6BaseName).arg(j);
			QImageWriter imageWriter{outImageBaseName + imageFormat};
			if (!imageWriter.canWrite()) {
				imageWriter.setFileName(outImageBaseName + defaultFormat);
				qWarning() << "can't save using the specified format, falling back to" << defaultFormat;
			}
			if (imageQuality > -1)
				imageWriter.setQuality(imageQuality);
			if (!imageWriter.write(image))
				qCritical() << "error saving output image:" << imageWriter.errorString();

			++j;
		}
	}
	return 0;
}
