/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2012 by Wenzel Jakob and Steve Marschner.

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/bitmap.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfVersion.h>
#include <ImfIO.h>

NORI_NAMESPACE_BEGIN

Bitmap::Bitmap(const QString &filename) {
	QByteArray filenameUtf8 = filename.toUtf8();
	Imf::InputFile file(filenameUtf8.data());
	const Imf::Header &header = file.header();
	const Imf::ChannelList &channels = header.channels();

	Imath::Box2i dw = file.header().dataWindow();
	resize(dw.max.y - dw.min.y + 1, dw.max.x - dw.min.x + 1);

	cout << "Reading a " << cols() << "x" << rows() 
		 << " OpenEXR file from \"" << qPrintable(filename) << "\"" << endl;
	
	const char *ch_r = NULL, *ch_g = NULL, *ch_b = NULL;
	for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it) {
		QString name = QString(it.name()).toLower();

		if (it.channel().xSampling != 1 || it.channel().ySampling != 1) {
			/* Sub-sampled layers are not supported */
			continue;
		}

		if (!ch_r && (name == "r" || name == "red" || 
				name.endsWith(".r") || name.endsWith(".red"))) {
			ch_r = it.name();
		} else if (!ch_g && (name == "g" || name == "green" || 
				name.endsWith(".g") || name.endsWith(".green"))) {
			ch_g = it.name();
		} else if (!ch_b && (name == "b" || name == "blue" || 
				name.endsWith(".b") || name.endsWith(".blue"))) {
			ch_b = it.name();
		}
	}

	if (!ch_r || !ch_g || !ch_b)
		throw NoriException("This is not a standard RGB OpenEXR file!");

	size_t compStride = sizeof(float),
	       pixelStride = 3 * compStride,
	       rowStride = pixelStride * cols();

	char *ptr = reinterpret_cast<char *>(data());

	Imf::FrameBuffer frameBuffer;
	frameBuffer.insert(ch_r, Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride)); ptr += compStride;
	frameBuffer.insert(ch_g, Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride)); ptr += compStride;
	frameBuffer.insert(ch_b, Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride)); 
	file.setFrameBuffer(frameBuffer);
	file.readPixels(dw.min.y, dw.max.y);
}

void Bitmap::save(const QString &filename) {
	cout << "Writing a " << cols() << "x" << rows() 
		 << " OpenEXR file to \"" << qPrintable(filename) << "\"" << endl;

	Imf::Header header(cols(), rows());
	header.insert("comments", Imf::StringAttribute("Generated by Nori"));

	Imf::ChannelList &channels = header.channels();
	channels.insert("R", Imf::Channel(Imf::FLOAT));
	channels.insert("G", Imf::Channel(Imf::FLOAT));
	channels.insert("B", Imf::Channel(Imf::FLOAT));

	Imf::FrameBuffer frameBuffer;
	size_t compStride = sizeof(float),
	       pixelStride = 3 * compStride,
	       rowStride = pixelStride * cols();

	char *ptr = reinterpret_cast<char *>(data());
	frameBuffer.insert("R", Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride)); ptr += compStride;
	frameBuffer.insert("G", Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride)); ptr += compStride;
	frameBuffer.insert("B", Imf::Slice(Imf::FLOAT, ptr, pixelStride, rowStride)); 

	QByteArray filenameUtf8 = filename.toUtf8();
	Imf::OutputFile file(filenameUtf8.data(), header);
	file.setFrameBuffer(frameBuffer);
	file.writePixels(rows());
}

NORI_NAMESPACE_END
