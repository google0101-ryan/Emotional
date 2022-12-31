#include "gs.h"
#include <cassert>

void GraphicsSynthesizer::process_vertex(uint64_t data, bool draw_kick)
{
	printf("Pushing vertex\n");

	Vertex v;
	v.coords.x() = data & 0xffff;
	v.coords.y() = (data >> 16) & 0xffff;
	v.coords.z() = (data >> 32) & 0xffffffff;

	v.col.r() = rgbaq.r;
	v.col.g() = rgbaq.g;
	v.col.b() = rgbaq.b;
	v.col.a() = rgbaq.a;

	if (!draw_kick && vqueue.size() == 3)
	{
		vqueue.pop_back();
	}
	
	vqueue.push_back(v);

	if (draw_kick)
	{
		if (vqueue.size() == 2 && (prim & 7) == 6)
		{
			vqueue[0].coords.x() -= xyoffset[0].x_offset;
			vqueue[1].coords.x() -= xyoffset[0].x_offset;
			vqueue[0].coords.y() -= xyoffset[0].y_offset;
			vqueue[1].coords.y() -= xyoffset[0].y_offset;

			Vertex v3, v4;
			v3.col = v4.col = vqueue[0].col;
			v3.coords.x() = vqueue[1].coords.x();
			v3.coords.y() = vqueue[0].coords.y();
			v4.coords.x() = vqueue[0].coords.x();
			v4.coords.y() = vqueue[1].coords.y();

			Vertex attribs[] =
			{
				vqueue[0], v3, vqueue[1],
				vqueue[1], v4, vqueue[0]
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(attribs), attribs, GL_STATIC_DRAW);
			OpenGL::draw(OpenGL::Primitives::Triangle, 6);
		
			printf("Queued vertices (%f, %f), (%f, %f), (%f, %f), (%f, %f)\n", (vqueue[0].coords.x() / 16.0f) / 256.0f - 1.0f, (vqueue[0].coords.y() / 16.0f) / 256.0f - 1.0f, (vqueue[1].coords.x() / 16.0f) / 256.0f - 1.0f, (vqueue[1].coords.y() / 16.0f) / 256.0f - 1.0f, (v3.coords.x() / 16.0f) / 256.0f - 1.0f, (v3.coords.y() / 16.0f) / 256.0f - 1.0f, (v4.coords.x() / 16.0f) / 256.0f - 1.0f, (v4.coords.y() / 16.0f) / 256.0f - 1.0f);
			printf("Normalized from (%f, %f) and (%f, %f)\n", vqueue[0].coords.x(), vqueue[0].coords.y(), vqueue[1].coords.x(), vqueue[1].coords.y());
			vqueue.clear();
		}
		else if (vqueue.size() == 3 && (prim & 7) == 3)
		{
			vqueue[0].coords.x() -= xyoffset[0].x_offset;
			vqueue[1].coords.x() -= xyoffset[0].x_offset;
			vqueue[2].coords.x() -= xyoffset[0].x_offset;
			vqueue[0].coords.y() -= xyoffset[0].y_offset;
			vqueue[1].coords.y() -= xyoffset[0].y_offset;
			vqueue[2].coords.y() -= xyoffset[0].y_offset;

			Vertex attribs[] =
			{
				vqueue[0], vqueue[1], vqueue[2]
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(attribs), attribs, GL_STATIC_DRAW);
			OpenGL::draw(OpenGL::Primitives::Triangle, 3);

			vqueue.clear();
		}
	}
}

void GraphicsSynthesizer::process_vertex_f(uint64_t data, bool draw_kick)
{
	printf("Pushing vertex\n");

	auto& xyz = draw_kick ? xyzf2 : xyzf3;

	Vertex v;
	v.coords.x() = xyz.x;
	v.coords.y() = xyz.y;
	v.coords.z() = xyz.z;

	v.col.r() = rgbaq.r;
	v.col.g() = rgbaq.g;
	v.col.b() = rgbaq.b;
	v.col.a() = rgbaq.a;
	
	if (!draw_kick && vqueue.size() == 3)
	{
		vqueue.pop_back();
	}
	
	vqueue.push_back(v);

	if (draw_kick)
	{
		if (vqueue.size() == 3 && (prim & 7) == 3)
		{
			// vqueue[0].coords.x() -= xyoffset[0].x_offset;
			// vqueue[1].coords.x() -= xyoffset[0].x_offset;
			// vqueue[2].coords.x() -= xyoffset[0].x_offset;
			// vqueue[0].coords.y() -= xyoffset[0].y_offset;
			// vqueue[1].coords.y() -= xyoffset[0].y_offset;
			// vqueue[2].coords.y() -= xyoffset[0].y_offset;

			Vertex attribs[] =
			{
				vqueue[0], vqueue[1], vqueue[2]
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(attribs), attribs, GL_STATIC_DRAW);
			OpenGL::draw(OpenGL::Primitives::Triangle, 3);

			vqueue.clear();
		}
	}
}

GraphicsSynthesizer::GraphicsSynthesizer()
{
	vram.create(2048, 2048, GL_RGB8);
	fb.createWithTexture(vram);
	fb.bind(OpenGL::FramebufferTypes::DrawAndReadFramebuffer);
	OpenGL::setViewport(640, 480);

	vbo.create();
	vbo.bind();
	vao.create();
	vao.bind();

	vao.setAttributeFloat<GLfloat>(0, 3, sizeof(Vertex), (void*)offsetof(Vertex, coords));
	vao.enableAttribute(0);
	vao.setAttributeInt<GLuint>(1, 4, sizeof(Vertex), (void*)offsetof(Vertex, col));
	vao.enableAttribute(1);

	if (!vertex_shader.create(vertex_shader_source, OpenGL::ShaderType::Vertex))
	{
		printf("Failed to compile vertex shader!\n");
		exit(1);
	}
	if (!fragment_shader.create(fragment_shader_source, OpenGL::ShaderType::Fragment))
	{
		printf("Failed to compile fragment shader!\n");
		exit(1);
	}
	if (!shader_program.create({vertex_shader, fragment_shader}))
	{
		printf("Failed to link shaders!\n");
		exit(1);
	}

	shader_program.use();

	offset_location = glGetUniformLocation(shader_program.handle(), "offsets");
}

void GraphicsSynthesizer::write(uint16_t index, uint64_t data)
{
	auto context = index & 1;
	switch (index)
	{
	case 0x0:
		prim = data;
		break;
	case 0x1:
		rgbaq.value = data;
		break;
	case 0x2:
		st = data;
		break;
	case 0x3:
		uv = data;
		break;
	case 0x4:
		xyzf2.value = data;
		process_vertex_f(data, true);
		break;
	case 0x5:
		xyz2.value = data;
		process_vertex(data, true);
		break;
	case 0x6:
	case 0x7:
		tex0[context] = data;
		break;
	case 0x8:
	case 0x9:
		clamp[context] = data;
		break;
	case 0xA:
		fog = data;
		break;
	case 0x0C:
		xyzf3.value = data;
		process_vertex_f(data, false);
		break;
	case 0xD:
		xyz3.value = data;
		process_vertex(data, false);
		break;
	case 0x14:
	case 0x15:
		tex1[context] = data;
		break;
	case 0x16:
	case 0x17:
		tex2[context] = data;
		break;
	case 0x18:
	case 0x19:
		xyoffset[context].value = data;
		glUniform2f(offset_location, (GLfloat)(xyoffset[context].x_offset), (GLfloat)(xyoffset[context].y_offset));
		break;
	case 0x1A:
		prmodecont = data;
		break;
	case 0x1B:
		prmode = data;
		break;
	case 0x1C:
		texclut = data;
		break;
	case 0x22:
		scanmsk = data;
		break;
	case 0x34:
	case 0x35:
		miptbp1[context] = data;
		break;
	case 0x36:
	case 0x37:
		miptbp2[context] = data;
		break;
	case 0x3B:
		texa = data;
		break;
	case 0x3d:
		fogcol = data;
		break;
	case 0x40:
	case 0x41:
		scissor[context] = data;
		break;
	case 0x42:
	case 0x43:
		alpha[context] = data;
		break;
	case 0x44:
		dimx = data;
		break;
	case 0x45:
		dthe = data;
		break;
	case 0x46:
		colclamp = data;
		break;
	case 0x47:
	case 0x48:
		test[context] = data;
		break;
	case 0x49:
		pabe = data;
		break;
	case 0x4a:
	case 0x4b:
		fba[context] = data;
		break;
	case 0x4c:
	case 0x4d:
		frame[context].value = data;
		break;
	case 0x4e:
	case 0x4f:
		zbuf[context] = data;
		break;
	case 0x50:
		bitbltbuf.value = data;
		break;
	case 0x51:
		trxpos.value = data;
		break;
	case 0x52:
		trxreg.value = data;
		vram_transfer_pixels = trxreg.height * trxreg.width;
		break;
	case 0x53:
    {
		trxdir = data;
		data_written = 0;
        break;
	}
	case 0x60:
		priv_regs.csr.signal = 1;
	case 0x61:
		priv_regs.csr.finish = 1;
		break;
	default:
		printf("Write to unknown register 0x%08x\n", index);
		exit(1);
	}
}

void GraphicsSynthesizer::write_hwreg(uint64_t data)
{
	vram_transfer_pixels--;
	transfer_buf.push_back(data & 0xffffffff);
	transfer_buf.push_back(data >> 32);
	printf("%d bytes remaining on VRAM transfer\n", vram_transfer_pixels);
	if (vram_transfer_pixels == 0)
	{
		vram.bind();
		glTexSubImage2D(GL_TEXTURE_2D, 0, trxpos.dest_top_left_x, trxpos.dest_top_left_y, trxreg.width, trxreg.height, GL_RGBA, GL_UNSIGNED_BYTE, &transfer_buf[0]);
		transfer_buf.clear();
	}
}