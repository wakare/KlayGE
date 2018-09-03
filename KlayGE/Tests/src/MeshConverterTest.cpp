/**
 * @file MeshConverterTest.cpp
 * @author Minmin Gong
 *
 * @section DESCRIPTION
 *
 * This source file is part of KlayGE
 * For the latest info, see http://www.klayge.org
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * You may alternatively use this source under the terms of
 * the KlayGE Proprietary License (KPL). You can obtained such a license
 * from http://www.klayge.org/licensing/.
 */

#include <KlayGE/KlayGE.hpp>
#include <KlayGE/Context.hpp>
#include <KlayGE/RenderFactory.hpp>
#include <KlayGE/ResLoader.hpp>
#include <KlayGE/Texture.hpp>
#include <KFL/CXX17/filesystem.hpp>
#include <KlayGE/Mesh.hpp>
#include <KlayGE/MeshConverter.hpp>
#include <KlayGE/MeshMetadata.hpp>

#include "KlayGETests.hpp"

using namespace std;
using namespace KlayGE;

class MeshConverterTest : public testing::Test
{
public:
	void SetUp() override
	{
		ResLoader::Instance().AddPath("../../Tests/media/MeshConverter");
	}

	void RunTest(std::string_view input_name, std::string_view metadata_name, std::string_view sanity_name)
	{
		MeshMetadata metadata(metadata_name);

		MeshConverter mc;
		auto target = mc.Convert(input_name, metadata);
		EXPECT_TRUE(target);

		auto sanity_model = LoadSoftwareModel(sanity_name);

		EXPECT_EQ(target->NumMaterials(), sanity_model->NumMaterials());
		for (uint32_t i = 0; i < target->NumMaterials(); ++ i)
		{
			auto mtl = target->GetMaterial(i);
			auto sanity_mtl = sanity_model->GetMaterial(i);

			EXPECT_EQ(mtl->name, sanity_mtl->name);
			EXPECT_FLOAT_EQ(mtl->albedo.x(), sanity_mtl->albedo.x());
			EXPECT_FLOAT_EQ(mtl->albedo.y(), sanity_mtl->albedo.y());
			EXPECT_FLOAT_EQ(mtl->albedo.z(), sanity_mtl->albedo.z());
			EXPECT_FLOAT_EQ(mtl->albedo.w(), sanity_mtl->albedo.w());
			EXPECT_FLOAT_EQ(mtl->metalness, sanity_mtl->metalness);
			EXPECT_FLOAT_EQ(mtl->glossiness, sanity_mtl->glossiness);
			EXPECT_FLOAT_EQ(mtl->emissive.x(), sanity_mtl->emissive.x());
			EXPECT_FLOAT_EQ(mtl->emissive.y(), sanity_mtl->emissive.y());
			EXPECT_FLOAT_EQ(mtl->emissive.z(), sanity_mtl->emissive.z());
			EXPECT_EQ(mtl->transparent, sanity_mtl->transparent);
			EXPECT_FLOAT_EQ(mtl->alpha_test, sanity_mtl->alpha_test);
			EXPECT_EQ(mtl->sss, sanity_mtl->sss);
			EXPECT_EQ(mtl->two_sided, sanity_mtl->two_sided);

			for (uint32_t slot = RenderMaterial::TS_Albedo; slot != RenderMaterial::TS_Height; ++ slot)
			{
				EXPECT_EQ(mtl->tex_names[slot], sanity_mtl->tex_names[slot]);
			}

			EXPECT_EQ(mtl->detail_mode, sanity_mtl->detail_mode);
			if (!mtl->tex_names[RenderMaterial::TS_Height].empty())
			{
				EXPECT_FLOAT_EQ(mtl->height_offset_scale.x(), sanity_mtl->height_offset_scale.x());
				EXPECT_FLOAT_EQ(mtl->height_offset_scale.y(), sanity_mtl->height_offset_scale.y());
			}
			if (mtl->detail_mode != RenderMaterial::SDM_Parallax)
			{
				EXPECT_FLOAT_EQ(mtl->tess_factors.x(), sanity_mtl->tess_factors.x());
				EXPECT_FLOAT_EQ(mtl->tess_factors.y(), sanity_mtl->tess_factors.y());
				EXPECT_FLOAT_EQ(mtl->tess_factors.z(), sanity_mtl->tess_factors.z());
				EXPECT_FLOAT_EQ(mtl->tess_factors.w(), sanity_mtl->tess_factors.w());
			}
		}

		auto const & rl = checked_cast<StaticMesh*>(target->Subrenderable(0).get())->GetRenderLayout();
		auto const & sanity_rl = checked_cast<StaticMesh*>(sanity_model->Subrenderable(0).get())->GetRenderLayout();

		EXPECT_EQ(rl.NumVertexStreams(), sanity_rl.NumVertexStreams());

		int position_stream = -1;
		int normal_stream = -1;
		int tangent_quat_stream = -1;
		int texcoord_stream = -1;
		int blend_weights_stream = -1;
		int blend_indices_stream = -1;
		{
			int stream_index = 0;
			for (uint32_t i = 0; i < rl.NumVertexStreams(); ++ i)
			{
				auto ve = rl.VertexStreamFormat(i)[0];

				if (ve.usage == VEU_Position)
				{
					position_stream = stream_index;
				}
				if ((ve.usage == VEU_Normal) && (ve.usage_index == 0) && (ve.format == EF_ABGR8))
				{
					normal_stream = stream_index;
				}
				if ((ve.usage == VEU_Tangent) && (ve.usage_index == 0) && (ve.format == EF_ABGR8))
				{
					tangent_quat_stream = stream_index;
				}
				if ((ve.usage == VEU_TextureCoord) && (ve.usage_index == 0) && (ve.format == EF_SIGNED_GR16))
				{
					texcoord_stream = stream_index;
				}
				if ((ve.usage == VEU_BlendWeight) && (ve.usage_index == 0) && (ve.format == EF_ABGR8))
				{
					blend_weights_stream = stream_index;
				}
				if ((ve.usage == VEU_BlendIndex) && (ve.usage_index == 0) && (ve.format == EF_ABGR8UI))
				{
					blend_indices_stream = stream_index;
				}

				++ stream_index;
			}
		}

		int sanity_position_stream = -1;
		int sanity_normal_stream = -1;
		int sanity_tangent_quat_stream = -1;
		int sanity_texcoord_stream = -1;
		int sanity_blend_weights_stream = -1;
		int sanity_blend_indices_stream = -1;
		{
			int stream_index = 0;
			for (uint32_t i = 0; i < sanity_rl.NumVertexStreams(); ++ i)
			{
				auto sanity_ve = sanity_rl.VertexStreamFormat(i)[0];

				if (sanity_ve.usage == VEU_Position)
				{
					sanity_position_stream = stream_index;
				}
				if ((sanity_ve.usage == VEU_Normal) && (sanity_ve.usage_index == 0) && (sanity_ve.format == EF_ABGR8))
				{
					sanity_normal_stream = stream_index;
				}
				if ((sanity_ve.usage == VEU_Tangent) && (sanity_ve.usage_index == 0) && (sanity_ve.format == EF_ABGR8))
				{
					sanity_tangent_quat_stream = stream_index;
				}
				if ((sanity_ve.usage == VEU_TextureCoord) && (sanity_ve.usage_index == 0) && (sanity_ve.format == EF_SIGNED_GR16))
				{
					sanity_texcoord_stream = stream_index;
				}
				if ((sanity_ve.usage == VEU_BlendWeight) && (sanity_ve.usage_index == 0) && (sanity_ve.format == EF_ABGR8))
				{
					sanity_blend_weights_stream = stream_index;
				}
				if ((sanity_ve.usage == VEU_BlendIndex) && (sanity_ve.usage_index == 0) && (sanity_ve.format == EF_ABGR8UI))
				{
					sanity_blend_indices_stream = stream_index;
				}

				++ stream_index;
			}
		}

		
		if (position_stream != -1)
		{
			EXPECT_NE(sanity_position_stream, -1);

			auto ve = rl.VertexStreamFormat(position_stream)[0];
			auto sanity_ve = sanity_rl.VertexStreamFormat(sanity_position_stream)[0];
			EXPECT_TRUE(ve == sanity_ve);
		}
		else
		{
			EXPECT_EQ(sanity_position_stream, -1);
		}
		if (normal_stream != -1)
		{
			EXPECT_NE(sanity_normal_stream, -1);

			auto ve = rl.VertexStreamFormat(normal_stream)[0];
			auto sanity_ve = sanity_rl.VertexStreamFormat(sanity_normal_stream)[0];
			EXPECT_TRUE(ve == sanity_ve);
		}
		else
		{
			EXPECT_EQ(sanity_normal_stream, -1);
		}
		if (tangent_quat_stream != -1)
		{
			EXPECT_NE(sanity_tangent_quat_stream, -1);

			auto ve = rl.VertexStreamFormat(tangent_quat_stream)[0];
			auto sanity_ve = sanity_rl.VertexStreamFormat(sanity_tangent_quat_stream)[0];
			EXPECT_TRUE(ve == sanity_ve);
		}
		else
		{
			EXPECT_EQ(sanity_tangent_quat_stream, -1);
		}
		if (texcoord_stream != -1)
		{
			EXPECT_NE(texcoord_stream, -1);

			auto ve = rl.VertexStreamFormat(texcoord_stream)[0];
			auto sanity_ve = sanity_rl.VertexStreamFormat(sanity_texcoord_stream)[0];
			EXPECT_TRUE(ve == sanity_ve);
		}
		else
		{
			EXPECT_EQ(texcoord_stream, -1);
		}
		if (blend_weights_stream != -1)
		{
			EXPECT_NE(sanity_blend_weights_stream, -1);

			auto ve = rl.VertexStreamFormat(blend_weights_stream)[0];
			auto sanity_ve = sanity_rl.VertexStreamFormat(sanity_blend_weights_stream)[0];
			EXPECT_TRUE(ve == sanity_ve);
		}
		else
		{
			EXPECT_EQ(sanity_blend_weights_stream, -1);
		}
		if (blend_indices_stream != -1)
		{
			EXPECT_NE(sanity_blend_indices_stream, -1);

			auto ve = rl.VertexStreamFormat(blend_indices_stream)[0];
			auto sanity_ve = sanity_rl.VertexStreamFormat(sanity_blend_indices_stream)[0];
			EXPECT_TRUE(ve == sanity_ve);
		}
		else
		{
			EXPECT_EQ(sanity_blend_indices_stream, -1);
		}
		EXPECT_EQ(rl.IndexStreamFormat(), sanity_rl.IndexStreamFormat());

		EXPECT_EQ(target->NumSubrenderables(), sanity_model->NumSubrenderables());
		for (uint32_t i = 0; i < target->NumSubrenderables(); ++ i)
		{
			auto const & mesh = *checked_cast<StaticMesh*>(target->Subrenderable(i).get());
			auto const & sanity_mesh = *checked_cast<StaticMesh*>(sanity_model->Subrenderable(i).get());

			EXPECT_EQ(mesh.MaterialID(), sanity_mesh.MaterialID());
			EXPECT_EQ(mesh.Name(), sanity_mesh.Name());
			EXPECT_EQ(mesh.NumLods(), sanity_mesh.NumLods());

			for (uint32_t lod = 0; lod < mesh.NumLods(); ++ lod)
			{
				EXPECT_EQ(mesh.NumVertices(lod), sanity_mesh.NumVertices(lod));
				EXPECT_EQ(mesh.NumIndices(lod), sanity_mesh.NumIndices(lod));

				auto const pos_center = mesh.PosBound().Center();
				auto const pos_extent = mesh.PosBound().HalfSize();
				auto const tc_center = mesh.TexcoordBound().Center();
				auto const tc_extent = mesh.TexcoordBound().HalfSize();

				auto const sanity_pos_center = sanity_mesh.PosBound().Center();
				auto const sanity_pos_extent = sanity_mesh.PosBound().HalfSize();
				auto const sanity_tc_center = sanity_mesh.TexcoordBound().Center();
				auto const sanity_tc_extent = sanity_mesh.TexcoordBound().HalfSize();

				GraphicsBuffer::Mapper position_mapper(*rl.GetVertexStream(position_stream), BA_Read_Only);
				auto const * position_buff = position_mapper.Pointer<int16_t>();
				
				GraphicsBuffer::Mapper sanity_position_mapper(*sanity_rl.GetVertexStream(sanity_position_stream), BA_Read_Only);
				auto const * sanity_position_buff = sanity_position_mapper.Pointer<int16_t>();

				uint8_t const * normal_buff = nullptr;
				uint8_t const * sanity_normal_buff = nullptr;
				if (normal_stream != -1)
				{
					GraphicsBuffer::Mapper normal_mapper(*rl.GetVertexStream(normal_stream), BA_Read_Only);
					normal_buff = normal_mapper.Pointer<uint8_t>();

					GraphicsBuffer::Mapper sanity_normal_mapper(*sanity_rl.GetVertexStream(sanity_normal_stream), BA_Read_Only);
					sanity_normal_buff = sanity_normal_mapper.Pointer<uint8_t>();
				}

				uint8_t const * tangent_buff = nullptr;
				uint8_t const * sanity_tangent_buff = nullptr;
				if (tangent_quat_stream != -1)
				{
					GraphicsBuffer::Mapper tangent_quat_mapper(*rl.GetVertexStream(tangent_quat_stream), BA_Read_Only);
					tangent_buff = tangent_quat_mapper.Pointer<uint8_t>();

					GraphicsBuffer::Mapper sanity_tangent_quat_mapper(
						*sanity_rl.GetVertexStream(sanity_tangent_quat_stream), BA_Read_Only);
					sanity_tangent_buff = sanity_tangent_quat_mapper.Pointer<uint8_t>();
				}

				int16_t const * texcoord_buff = nullptr;
				int16_t const * sanity_texcoord_buff = nullptr;
				if (texcoord_stream != -1)
				{
					GraphicsBuffer::Mapper texcoord_mapper(*rl.GetVertexStream(texcoord_stream), BA_Read_Only);
					texcoord_buff = texcoord_mapper.Pointer<int16_t>();

					GraphicsBuffer::Mapper sanity_texcoord_mapper(*sanity_rl.GetVertexStream(sanity_texcoord_stream), BA_Read_Only);
					sanity_texcoord_buff = sanity_texcoord_mapper.Pointer<int16_t>();
				}

				uint8_t const * blend_weights_buff = nullptr;
				uint8_t const * sanity_blend_weights_buff = nullptr;
				if (blend_weights_stream != -1)
				{
					GraphicsBuffer::Mapper blend_weights_mapper(*rl.GetVertexStream(blend_weights_stream), BA_Read_Only);
					blend_weights_buff = blend_weights_mapper.Pointer<uint8_t>();

					GraphicsBuffer::Mapper sanity_blend_weights_mapper(*sanity_rl.GetVertexStream(sanity_blend_weights_stream),
						BA_Read_Only);
					sanity_blend_weights_buff = sanity_blend_weights_mapper.Pointer<uint8_t>();
				}
				
				uint8_t const * blend_indices_buff = nullptr;
				uint8_t const * sanity_blend_indices_buff = nullptr;
				if (blend_weights_stream != -1)
				{
					GraphicsBuffer::Mapper blend_indices_mapper(*rl.GetVertexStream(blend_indices_stream), BA_Read_Only);
					blend_indices_buff = blend_indices_mapper.Pointer<uint8_t>();

					GraphicsBuffer::Mapper sanity_blend_indices_mapper(*sanity_rl.GetVertexStream(sanity_blend_indices_stream),
						BA_Read_Only);
					sanity_blend_indices_buff = sanity_blend_indices_mapper.Pointer<uint8_t>();
				}

				EXPECT_EQ(mesh.NumVertices(lod), sanity_mesh.NumVertices(lod));
				EXPECT_EQ(mesh.StartVertexLocation(lod), sanity_mesh.StartVertexLocation(lod));
				EXPECT_EQ(mesh.NumIndices(lod), sanity_mesh.NumIndices(lod));
				EXPECT_EQ(mesh.StartIndexLocation(lod), sanity_mesh.StartIndexLocation(lod));

				for (uint32_t vid = 0; vid < sanity_mesh.NumVertices(lod); ++ vid)
				{
					uint32_t const index = vid + sanity_mesh.StartVertexLocation(lod);

					{
						float3 pos;
						pos.x() = ((position_buff[index * 4 + 0] + 32768) / 65535.0f * 2 - 1)
							* pos_extent.x() + pos_center.x();
						pos.y() = ((position_buff[index * 4 + 1] + 32768) / 65535.0f * 2 - 1)
							* pos_extent.y() + pos_center.y();
						pos.z() = ((position_buff[index * 4 + 2] + 32768) / 65535.0f * 2 - 1)
							* pos_extent.z() + pos_center.z();

						float3 sanity_pos;
						sanity_pos.x() = ((sanity_position_buff[index * 4 + 0] + 32768) / 65535.0f * 2 - 1)
							* sanity_pos_extent.x() + sanity_pos_center.x();
						sanity_pos.y() = ((sanity_position_buff[index * 4 + 1] + 32768) / 65535.0f * 2 - 1)
							* sanity_pos_extent.y() + sanity_pos_center.y();
						sanity_pos.z() = ((sanity_position_buff[index * 4 + 2] + 32768) / 65535.0f * 2 - 1)
							* sanity_pos_extent.z() + sanity_pos_center.z();

						EXPECT_TRUE(std::abs(pos.x() - sanity_pos.x()) < 1e-3f);
						EXPECT_TRUE(std::abs(pos.y() - sanity_pos.y()) < 1e-3f);
						EXPECT_TRUE(std::abs(pos.z() - sanity_pos.z()) < 1e-3f);
					}

					if (normal_stream != -1)
					{
						float4 normal;
						normal.x() = (normal_buff[index * 4 + 0] / 255.0f) * 2 - 1;
						normal.y() = (normal_buff[index * 4 + 1] / 255.0f) * 2 - 1;
						normal.z() = (normal_buff[index * 4 + 2] / 255.0f) * 2 - 1;
						normal = MathLib::normalize(normal);

						float4 sanity_normal;
						sanity_normal.x() = (sanity_normal_buff[index * 4 + 0] / 255.0f) * 2 - 1;
						sanity_normal.y() = (sanity_normal_buff[index * 4 + 1] / 255.0f) * 2 - 1;
						sanity_normal.z() = (sanity_normal_buff[index * 4 + 2] / 255.0f) * 2 - 1;
						sanity_normal = MathLib::normalize(sanity_normal);

						EXPECT_TRUE(std::abs(normal.x() - sanity_normal.x()) < 2e-2f);
						EXPECT_TRUE(std::abs(normal.y() - sanity_normal.y()) < 2e-2f);
						EXPECT_TRUE(std::abs(normal.z() - sanity_normal.z()) < 2e-2f);
					}

					if (tangent_quat_stream != -1)
					{
						float4 tangent;
						tangent.x() = (tangent_buff[index * 4 + 0] / 255.0f) * 2 - 1;
						tangent.y() = (tangent_buff[index * 4 + 1] / 255.0f) * 2 - 1;
						tangent.z() = (tangent_buff[index * 4 + 2] / 255.0f) * 2 - 1;
						tangent.w() = (tangent_buff[index * 4 + 3] / 255.0f) * 2 - 1;
						tangent = MathLib::normalize(tangent);

						float4 sanity_tangent;
						sanity_tangent.x() = (sanity_tangent_buff[index * 4 + 0] / 255.0f) * 2 - 1;
						sanity_tangent.y() = (sanity_tangent_buff[index * 4 + 1] / 255.0f) * 2 - 1;
						sanity_tangent.z() = (sanity_tangent_buff[index * 4 + 2] / 255.0f) * 2 - 1;
						sanity_tangent.w() = (sanity_tangent_buff[index * 4 + 3] / 255.0f) * 2 - 1;
						sanity_tangent = MathLib::normalize(sanity_tangent);

						EXPECT_TRUE(std::abs(tangent.x() - sanity_tangent.x()) < 2e-2f);
						EXPECT_TRUE(std::abs(tangent.y() - sanity_tangent.y()) < 2e-2f);
						EXPECT_TRUE(std::abs(tangent.z() - sanity_tangent.z()) < 2e-2f);
						EXPECT_TRUE(std::abs(tangent.w() - sanity_tangent.w()) < 2e-2f);
					}

					if (texcoord_stream != -1)
					{
						float3 tc;
						tc.x() = ((texcoord_buff[index * 2 + 0] + 32768) / 65535.0f * 2 - 1) * tc_extent.x() + tc_center.x();
						tc.y() = ((texcoord_buff[index * 2 + 1] + 32768) / 65535.0f * 2 - 1) * tc_extent.y() + tc_center.y();

						float3 sanity_tc;
						sanity_tc.x() = ((sanity_texcoord_buff[index * 2 + 0] + 32768) / 65535.0f * 2 - 1)
							* sanity_tc_extent.x() + sanity_tc_center.x();
						sanity_tc.y() = ((sanity_texcoord_buff[index * 2 + 1] + 32768) / 65535.0f * 2 - 1)
							* sanity_tc_extent.y() + sanity_tc_center.y();

						EXPECT_TRUE(std::abs(tc.x() - sanity_tc.x()) < 2e-3f);
						EXPECT_TRUE(std::abs(tc.y() - sanity_tc.y()) < 2e-3f);
					}

					if (blend_weights_stream != -1)
					{
						EXPECT_NE(sanity_blend_weights_stream, -1);
						EXPECT_NE(blend_indices_stream, -1);
						EXPECT_NE(sanity_blend_indices_stream, -1);

						for (uint32_t wi = 0; wi < 4; ++ wi)
						{
							EXPECT_EQ(blend_weights_buff[index * 4 + wi], sanity_blend_weights_buff[index * 4 + wi]);
							if (blend_weights_buff[index * 4 + wi] > 0)
							{
								EXPECT_EQ(blend_indices_buff[index * 4 + wi], sanity_blend_indices_buff[index * 4 + wi]);
							}
						}
					}
				}

				GraphicsBuffer::Mapper indices_mapper(*rl.GetIndexStream(), BA_Read_Only);
				auto const * indices_buff_16 = indices_mapper.Pointer<uint16_t>();
				auto const * indices_buff_32 = indices_mapper.Pointer<uint32_t>();

				GraphicsBuffer::Mapper sanity_indices_mapper(*sanity_rl.GetIndexStream(), BA_Read_Only);
				auto const * sanity_indices_buff_16 = sanity_indices_mapper.Pointer<uint16_t>();
				auto const * sanity_indices_buff_32 = sanity_indices_mapper.Pointer<uint32_t>();

				for (uint32_t iid = 0; iid < sanity_mesh.NumIndices(lod); ++ iid)
				{
					uint32_t const index = iid + sanity_mesh.StartIndexLocation(lod);
					uint32_t tri_index;
					uint32_t sanity_tri_index;
					if (sanity_rl.IndexStreamFormat() == EF_R16UI)
					{
						tri_index = indices_buff_16[index];
						sanity_tri_index = sanity_indices_buff_16[index];
					}
					else
					{
						tri_index = indices_buff_32[index];
						sanity_tri_index = sanity_indices_buff_32[index];
					}

					EXPECT_EQ(tri_index, sanity_tri_index);
				}
			}
		}

		EXPECT_EQ(target->IsSkinned(), sanity_model->IsSkinned());
		if (sanity_model->IsSkinned())
		{
			auto& skinned_model = *checked_cast<SkinnedModel*>(target.get());
			auto& sanity_skinned_model = *checked_cast<SkinnedModel*>(sanity_model.get());

			EXPECT_EQ(skinned_model.NumJoints(), sanity_skinned_model.NumJoints());
			for (uint32_t i = 0; i < sanity_skinned_model.NumJoints(); ++ i)
			{
				auto& joint = skinned_model.GetJoint(i);
				auto& sanity_joint = sanity_skinned_model.GetJoint(i);

				EXPECT_EQ(joint.name, sanity_joint.name);
				EXPECT_EQ(joint.parent, sanity_joint.parent);

				EXPECT_TRUE(std::abs(joint.bind_real.x() - sanity_joint.bind_real.x()) < 1e-4f);
				EXPECT_TRUE(std::abs(joint.bind_real.y() - sanity_joint.bind_real.y()) < 1e-4f);
				EXPECT_TRUE(std::abs(joint.bind_real.z() - sanity_joint.bind_real.z()) < 1e-4f);
				EXPECT_TRUE(std::abs(joint.bind_real.w() - sanity_joint.bind_real.w()) < 1e-4f);

				EXPECT_TRUE(std::abs(joint.bind_dual.x() - sanity_joint.bind_dual.x()) < 1e-4f);
				EXPECT_TRUE(std::abs(joint.bind_dual.y() - sanity_joint.bind_dual.y()) < 1e-4f);
				EXPECT_TRUE(std::abs(joint.bind_dual.z() - sanity_joint.bind_dual.z()) < 1e-4f);
				EXPECT_TRUE(std::abs(joint.bind_dual.w() - sanity_joint.bind_dual.w()) < 1e-4f);

				EXPECT_TRUE(std::abs(joint.bind_scale - sanity_joint.bind_scale) < 1e-5f);
			}

			EXPECT_EQ(skinned_model.NumActions(), sanity_skinned_model.NumActions());
			for (uint32_t i = 0; i < sanity_skinned_model.NumActions(); ++ i)
			{
				std::string action_name;
				uint32_t start_frame;
				uint32_t end_frame;
				skinned_model.GetAction(i, action_name, start_frame, end_frame);

				std::string sanity_action_name;
				uint32_t sanity_start_frame;
				uint32_t sanity_end_frame;
				sanity_skinned_model.GetAction(i, sanity_action_name, sanity_start_frame, sanity_end_frame);

				EXPECT_EQ(action_name, sanity_action_name);
				EXPECT_EQ(start_frame, sanity_start_frame);
				EXPECT_EQ(end_frame, sanity_end_frame);
			}

			EXPECT_EQ(skinned_model.GetKeyFrameSets()->size(), sanity_skinned_model.GetKeyFrameSets()->size());
			for (uint32_t i = 0; i < sanity_skinned_model.GetKeyFrameSets()->size(); ++ i)
			{
				auto const & key_frames = (*(skinned_model.GetKeyFrameSets()))[i];
				auto const & sanity_key_frames = (*(sanity_skinned_model.GetKeyFrameSets()))[i];

				EXPECT_EQ(key_frames.frame_id.size(), key_frames.bind_real.size());
				EXPECT_EQ(key_frames.frame_id.size(), key_frames.bind_dual.size());
				EXPECT_EQ(key_frames.frame_id.size(), key_frames.bind_scale.size());

				if (key_frames.frame_id == sanity_key_frames.frame_id)
				{
					for (uint32_t j = 0; j < key_frames.frame_id.size(); ++ j)
					{
						auto const & bind_real = sanity_key_frames.bind_real[j];
						auto const & bind_dual = sanity_key_frames.bind_dual[j];
						float bind_scale = sanity_key_frames.bind_scale[j];

						auto const & sanity_bind_real = sanity_key_frames.bind_real[j];
						auto const & sanity_bind_dual = sanity_key_frames.bind_dual[j];
						float sanity_bind_scale = sanity_key_frames.bind_scale[j];

						EXPECT_TRUE(std::abs(bind_real.x() - sanity_bind_real.x()) < 1e-5f);
						EXPECT_TRUE(std::abs(bind_real.y() - sanity_bind_real.y()) < 1e-5f);
						EXPECT_TRUE(std::abs(bind_real.z() - sanity_bind_real.z()) < 1e-5f);
						EXPECT_TRUE(std::abs(bind_real.w() - sanity_bind_real.w()) < 1e-5f);

						EXPECT_TRUE(std::abs(bind_dual.x() - sanity_bind_dual.x()) < 1e-5f);
						EXPECT_TRUE(std::abs(bind_dual.y() - sanity_bind_dual.y()) < 1e-5f);
						EXPECT_TRUE(std::abs(bind_dual.z() - sanity_bind_dual.z()) < 1e-5f);
						EXPECT_TRUE(std::abs(bind_dual.w() - sanity_bind_dual.w()) < 1e-5f);

						EXPECT_TRUE(std::abs(bind_scale - sanity_bind_scale) < 1e-4f);
					}
				}
				else
				{
					for (uint32_t j = 0; j < key_frames.frame_id.size(); ++ j)
					{
						uint32_t const frame_id = key_frames.frame_id[j];

						Quaternion bind_real;
						Quaternion bind_dual;
						float bind_scale;
						std::tie(bind_real, bind_dual, bind_scale) = sanity_key_frames.Frame(static_cast<float>(frame_id));

						Quaternion sanity_bind_real;
						Quaternion sanity_bind_dual;
						float sanity_bind_scale;
						std::tie(sanity_bind_real, sanity_bind_dual, sanity_bind_scale)
							= sanity_key_frames.Frame(static_cast<float>(frame_id));

						EXPECT_TRUE(std::abs(bind_real.x() - sanity_bind_real.x()) < 1e-4f);
						EXPECT_TRUE(std::abs(bind_real.y() - sanity_bind_real.y()) < 1e-4f);
						EXPECT_TRUE(std::abs(bind_real.z() - sanity_bind_real.z()) < 1e-4f);
						EXPECT_TRUE(std::abs(bind_real.w() - sanity_bind_real.w()) < 1e-4f);

						EXPECT_TRUE(std::abs(bind_dual.x() - sanity_bind_dual.x()) < 1e-4f);
						EXPECT_TRUE(std::abs(bind_dual.y() - sanity_bind_dual.y()) < 1e-4f);
						EXPECT_TRUE(std::abs(bind_dual.z() - sanity_bind_dual.z()) < 1e-4f);
						EXPECT_TRUE(std::abs(bind_dual.w() - sanity_bind_dual.w()) < 1e-4f);

						EXPECT_TRUE(std::abs(bind_scale - sanity_bind_scale) < 1e-4f);
					}
				}
			}

			EXPECT_EQ(skinned_model.NumFrames(), sanity_skinned_model.NumFrames());
			EXPECT_EQ(skinned_model.FrameRate(), sanity_skinned_model.FrameRate());
		}
	}
};

TEST_F(MeshConverterTest, StaticNoLod)
{
	RunTest("tree2a_lod0.obj", "tree2a.nolod.kmeta", "tree2a.nolod.meshml");
}

TEST_F(MeshConverterTest, StaticLod)
{
	RunTest("tree2a_lod0.obj", "tree2a.lod.kmeta", "tree2a.lod.meshml");
}

TEST_F(MeshConverterTest, StaticLodAutoCenter)
{
	RunTest("tree2a_lod0.obj", "tree2a.lod_autocenter.kmeta", "tree2a.lod_autocenter.meshml");
}

TEST_F(MeshConverterTest, StaticLodAxisMapping)
{
	RunTest("tree2a_lod0.obj", "tree2a.lod_axismapping.kmeta", "tree2a.lod_axismapping.meshml");
}

TEST_F(MeshConverterTest, StaticLodTransforms)
{
	RunTest("tree2a_lod0.obj", "tree2a.lod_trans.kmeta", "tree2a.lod_trans.meshml");
}

TEST_F(MeshConverterTest, AnimationPassThrough)
{
	RunTest("anim.fbx", "", "anim.meshml");
}

TEST_F(MeshConverterTest, StaticMeshML)
{
	RunTest("tree2a.lod.meshml", "", "tree2a.lod.meshml");
}

TEST_F(MeshConverterTest, AnimationMeshML)
{
	RunTest("anim.meshml", "", "anim.meshml");
}
