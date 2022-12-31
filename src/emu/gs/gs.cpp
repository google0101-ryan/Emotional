#include "gs.h"
#include <cassert>

void GraphicsSynthesizer::submit_vertex(XYZ xyz, bool draw_kick)
{
	Vertex vertex;
	vertex.position = glm::vec3(xyz.x, xyz.y, xyz.z);
	process_vertex(vertex, draw_kick);
}

void GraphicsSynthesizer::submit_vertex_fog(XYZF xyzf, bool draw_kick)
{
	Vertex vertex;
	vertex.position = glm::vec3(xyzf.x, xyzf.y, xyzf.z);
	process_vertex(vertex, draw_kick);
}

void GraphicsSynthesizer::process_vertex(Vertex vertex, bool draw_kick)
{
	auto& pos = vertex.position;
	pos.x = (pos.x - xyoffset[0].x_offset) / 16.0f;
	pos.y = (pos.y - xyoffset[0].y_offset) / 16.0f;

	pos.x = (pos.x / 320.0f) - 1.0f;
	pos.y = 1.0f - (pos.y / 112.0f);
	pos.z = pos.z / static_cast<float>(INT_MAX);

	vertex.color = glm::vec3(rgbaq.r, rgbaq.g, rgbaq.b) / 255.0f;

	vqueue.push(vertex);

	if (draw_kick)
	{
		int size = 0;
		switch (prim & 7)
		{
		case 0:
			size = 1;
			break;
		case 1:
		case 2:
			size = 2;
			break;
		case 3:
		case 4:
		case 5:
			size = 3;
			break;
		case 6:
			size = 2;
			break;
		}

		if (size && size <= vqueue.size())
		{
			switch ((prim & 7))
			{
			case 6:
			{
				assert(vqueue.size() == 2);
				auto& v1 = vqueue.front();
				vqueue.pop();
				auto& v2 = vqueue.front();
				vqueue.pop();
				Vertex v3 = v1;
				v3.position.y = v2.position.y;
				Vertex v4 = v2;
				v4.position.x = v1.position.x;

				printf("Pushing sprite at (%f, %f, %f), (%f, %f, %f)\n", v1.position.x, v1.position.y, v1.position.z, v2.position.x, v2.position.y, v2.position.z);

				renderer->vertices.emplace_back(v1);
				renderer->vertices.emplace_back(v2);
				renderer->vertices.emplace_back(v3);
				renderer->vertices.emplace_back(v4);
				break;
			}
			}
		}
	}
}

GraphicsSynthesizer::GraphicsSynthesizer(Renderer *renderer)
: renderer(renderer)
{
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
		submit_vertex_fog(xyzf2, true);
		break;
	case 0x5:
		xyz2.value = data;
		submit_vertex(xyz2, true);
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
	case 0xD:
		xyz3.value = data;
		submit_vertex(xyz3, false);
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
		break;
	case 0x53:
    {
		trxdir = data;
		data_written = 0;
        break;
	}
	case 0x61:
		priv_regs.csr.finish = 1;
		break;
	default:
		printf("Write to unknown register 0x%08x\n", index);
		exit(1);
	}
}

