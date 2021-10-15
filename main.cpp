#include <QColor>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QImageWriter>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#define QAPPLICATION_CLASS QGuiApplication
#else
#include <QApplication>
#define QAPPLICATION_CLASS QApplication
#endif

#include <cstdint>
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


int main(int argc, char* argv[])
{
	const auto dc6Path = argv[1];
	const auto palettePath = argv[2];

	QFile f{dc6Path};
	if (!f.open(QFile::ReadOnly)) {
		qCritical() << "error opening dc6 file:" << f.errorString();
		return 1;
	}

	QFile paletteFile{palettePath};
	if (!paletteFile.open(QFile::ReadOnly)) {
		qCritical() << "error opening palette file:" << paletteFile.errorString();
		return 1;
	}

	QAPPLICATION_CLASS app{argc, argv};
	qDebug() << QImageWriter::supportedImageFormats();

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

	ds.setDevice(&f);
	ds.setByteOrder(QDataStream::LittleEndian);

	Dc6Header header;
	ds >> header.alwaysSix;
	ds >> header.alwaysOne;
	ds >> header.alwaysZero;
	if (header.alwaysSix != 6 || header.alwaysOne != 1 || header.alwaysZero != 0) {
		qCritical() << "invalid header";
		return 1;
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
		for (const auto ext : {"png", "jpg"}) {
			QImageWriter imageWriter{QString("%1.%2").arg(j).arg(ext), ext};
			// imageWriter.setQuality(100);
			if (!imageWriter.write(image))
				qCritical() << imageWriter.errorString();
		}

		++j;
	}
	return 0;
}
