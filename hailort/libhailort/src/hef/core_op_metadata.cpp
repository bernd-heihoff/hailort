/**
 * Copyright (c) 2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the MIT license (https://opensource.org/licenses/MIT)
**/
/**
 * @file core_op_metadata.cpp
 * @brief Contains all relevant information about a core-op from the hef.
 **/

#include "core_op_metadata.hpp"
#include <numeric>

namespace hailort
{

static void get_demuxes_names_impl(const LayerInfo &info, std::vector<std::string> &res)
{
    if (!info.is_mux) {
        res.push_back(info.name);
    } else {
        for (auto &pred : info.predecessor) {
            get_demuxes_names_impl(pred, res);
        }
    }
}

static std::vector<std::string> get_demuxes_names(const LayerInfo &info)
{
    std::vector<std::string> res;
    get_demuxes_names_impl(info, res);
    return res;
}

static bool is_edge_under_mux(const LayerInfo &info, const std::string &edge_name)
{
    if (!info.is_mux) {
        return edge_name == info.name;
    }
    for (const auto &pred : info.predecessor) {
        if (info.is_mux) {
            if (is_edge_under_mux(pred, edge_name)) {
                return true;
            }
        } else {
            if (edge_name == pred.name) {
                return true;
            }
        }
    }
    return false;
}

ContextMetadata::ContextMetadata(std::vector<ContextSwitchConfigActionPtr> &&actions,
    ConfigBufferInfoMap&& config_buffers_info) :
    m_actions(std::move(actions)),
    m_config_buffers_info(std::move(config_buffers_info))
{}

const ConfigBufferInfoMap &ContextMetadata::config_buffers_info() const
{
    return m_config_buffers_info;
}

const std::vector<ContextSwitchConfigActionPtr> &ContextMetadata::get_actions() const
{
    return m_actions;
}

std::vector<ContextSwitchConfigActionPtr> ContextMetadata::get_actions_of_type(
    const std::set<ContextSwitchConfigAction::Type> &action_types) const
{
    std::vector<ContextSwitchConfigActionPtr> filtered_actions;
    for (const auto &action : m_actions) {
        if (action_types.find(action->get_type()) != action_types.end()) {
            filtered_actions.emplace_back(action);
        }
    }
    return filtered_actions;
}

void ContextMetadata::add_boundary_layer(const LayerInfo &layer_info)
{
    if (HAILO_H2D_STREAM == layer_info.direction) {
        m_boundary_input_layers.push_back(layer_info);
    } else {
        m_boundary_output_layers.push_back(layer_info);
    }
}

void ContextMetadata::add_inter_context_layer(const LayerInfo &layer_info)
{
    if (HAILO_H2D_STREAM == layer_info.direction) {
        m_inter_context_input_layers.push_back(layer_info);
    } else {
        m_inter_context_output_layers.push_back(layer_info);
    }
}

void ContextMetadata::add_ddr_layer(const LayerInfo &layer_info)
{
    if (HAILO_H2D_STREAM == layer_info.direction) {
        m_ddr_input_layers.push_back(layer_info);
    } else {
        m_ddr_output_layers.push_back(layer_info);
    }
}

const std::vector<LayerInfo> &ContextMetadata::get_boundary_input_layers() const
{
    return m_boundary_input_layers;
}

const std::vector<LayerInfo> &ContextMetadata::get_boundary_output_layers() const
{
    return m_boundary_output_layers;
}

const std::vector<LayerInfo> &ContextMetadata::get_inter_context_input_layers() const
{
    return m_inter_context_input_layers;
}

const std::vector<LayerInfo> &ContextMetadata::get_inter_context_output_layers() const
{
    return m_inter_context_output_layers;
}

const std::vector<LayerInfo> &ContextMetadata::get_ddr_input_layers() const
{
    return m_ddr_input_layers;
}

const std::vector<LayerInfo> &ContextMetadata::get_ddr_output_layers() const
{
    return m_ddr_output_layers;
}

Expected<size_t> ContextMetadata::get_layers_transfer_size(const std::vector<LayerInfo> &layer_infos) const
{
    size_t total_transfer_size = 0;
    for (const auto &layer_info : layer_infos) {
        auto transfer_size = LayerInfoUtils::get_transfer_size(layer_info);
        CHECK_EXPECTED(transfer_size);
        total_transfer_size += transfer_size.release();
    }
    return total_transfer_size;
}

Expected<size_t> ContextMetadata::get_context_transfer_size() const
{
    size_t total_transfer_size = 0;

    // Calc config buffers 
    for (const auto &config_buffer_sizes : m_config_buffers_info) {
        total_transfer_size += std::accumulate(config_buffer_sizes.second.begin(), config_buffer_sizes.second.end(), 0);
    }

    // Calc all edge layers
    auto boundary_input_transfer_size = get_layers_transfer_size(m_boundary_input_layers);
    CHECK_EXPECTED(boundary_input_transfer_size);
    auto boundary_output_transfer_size = get_layers_transfer_size(m_boundary_output_layers);
    CHECK_EXPECTED(boundary_output_transfer_size);
    auto ddr_input_transfer_size = get_layers_transfer_size(m_ddr_input_layers);
    CHECK_EXPECTED(ddr_input_transfer_size);
    auto ddr_output_transfer_size = get_layers_transfer_size(m_ddr_output_layers);
    CHECK_EXPECTED(ddr_output_transfer_size);
    auto inter_context_input_transfer_size = get_layers_transfer_size(m_inter_context_input_layers);
    CHECK_EXPECTED(inter_context_input_transfer_size);
    auto inter_context_output_transfer_size = get_layers_transfer_size(m_inter_context_output_layers);
    CHECK_EXPECTED(inter_context_output_transfer_size);

    total_transfer_size += 
        boundary_input_transfer_size.release() + boundary_output_transfer_size.release() + 
        ddr_input_transfer_size.release() + ddr_output_transfer_size.release() + 
        inter_context_input_transfer_size.release() + inter_context_output_transfer_size.release();

    return total_transfer_size;
}

CoreOpMetadata::CoreOpMetadata(const std::string &core_op_name,
    ContextMetadata &&preliminary_context,
    std::vector<ContextMetadata> &&dynamic_contexts,
    std::vector<ConfigChannelInfo> &&config_channels_info,
    std::vector<std::string> &&sorted_output_names,
    SupportedFeatures &supported_features,
    const std::vector<std::string> &sorted_network_names)
    :   m_preliminary_context(std::move(preliminary_context)),
        m_dynamic_contexts(std::move(dynamic_contexts)),
        m_config_channels_info(std::move(config_channels_info)),
        m_core_op_name(core_op_name), m_sorted_output_names(std::move(sorted_output_names)),
        m_supported_features(supported_features), m_sorted_network_names(sorted_network_names) {}

Expected<LayerInfo> CoreOpMetadata::get_layer_info_by_stream_name(const std::string &stream_name) const
{
    for (auto layer_info : get_all_layer_infos()) {
        if (layer_info.name == stream_name) {
            return layer_info;
        }
    }
    LOGGER__ERROR("Failed to find layer with name {}", stream_name);
    return make_unexpected(HAILO_NOT_FOUND);
}

std::vector<LayerInfo> CoreOpMetadata::get_input_layer_infos() const
{
    std::vector<LayerInfo> res;
    // Edge layers exists only in the dynamic context.
    for (const auto &context : m_dynamic_contexts) {
        for (const auto &layer_info : context.get_boundary_input_layers()) {
            res.emplace_back(layer_info);
        }
    }
    return res;
}

std::vector<LayerInfo> CoreOpMetadata::get_output_layer_infos() const
{
    std::vector<LayerInfo> res;
    // Edge layers exists only in the dynamic context.
    for (const auto &context : m_dynamic_contexts) {
        for (const auto &layer_info : context.get_boundary_output_layers()) {
            res.emplace_back(layer_info);
        }
    }
    return res;
}

std::vector<LayerInfo> CoreOpMetadata::get_all_layer_infos() const
{
    const auto input_layer_infos = get_input_layer_infos();
    const auto output_layer_infos = get_output_layer_infos();

    std::vector<LayerInfo> res;
    res.reserve(input_layer_infos.size() + output_layer_infos.size());
    res.insert(res.end(), input_layer_infos.begin(), input_layer_infos.end());
    res.insert(res.end(), output_layer_infos.begin(), output_layer_infos.end());

    return res;
}

Expected<std::vector<LayerInfo>> CoreOpMetadata::get_input_layer_infos(const std::string &network_name) const
{
    std::vector<LayerInfo> res;
    // Edge layers exists only in the dynamic context.
    for (const auto &context : m_dynamic_contexts) {
        for (const auto &layer_info : context.get_boundary_input_layers()) {
            if ((layer_info.network_name == network_name) || (network_name.empty()) || (network_name == default_network_name())) {
                res.emplace_back(layer_info);
            }
        }
    }
    CHECK_AS_EXPECTED(res.size() > 0, HAILO_NOT_FOUND, "Network name {} is not found in networks metadata", network_name);
    return res;
}

Expected<std::vector<LayerInfo>> CoreOpMetadata::get_output_layer_infos(const std::string &network_name) const
{
    std::vector<LayerInfo> res;
    // Edge layers exists only in the dynamic context.
    for (const auto &context : m_dynamic_contexts) {
        for (auto &layer_info : context.get_boundary_output_layers()) {
            if ((layer_info.network_name == network_name) || (network_name.empty()) || (network_name == default_network_name())) {
                res.emplace_back(layer_info);
            }
        }
    }
    CHECK_AS_EXPECTED(res.size() > 0, HAILO_NOT_FOUND, "Network name {} is not found in networks metadata", network_name);
    return res;
}

const ContextMetadata &CoreOpMetadata::preliminary_context() const
{
    return m_preliminary_context;
}

const std::vector<ContextMetadata> &CoreOpMetadata::dynamic_contexts() const
{
    return m_dynamic_contexts;
}

const std::vector<ConfigChannelInfo> &CoreOpMetadata::config_channels_info() const
{
    return m_config_channels_info;
}

Expected<std::vector<LayerInfo>> CoreOpMetadata::get_all_layer_infos(const std::string &network_name) const
{
    auto input_layer_infos = get_input_layer_infos(network_name);
    CHECK_EXPECTED(input_layer_infos);

    auto output_layer_infos = get_output_layer_infos(network_name);
    CHECK_EXPECTED(output_layer_infos);

    std::vector<LayerInfo> res;
    res.reserve(input_layer_infos->size() + output_layer_infos->size());
    res.insert(res.end(), input_layer_infos->begin(), input_layer_infos->end());
    res.insert(res.end(), output_layer_infos->begin(), output_layer_infos->end());

    return res;
}

Expected<std::vector<hailo_stream_info_t>> CoreOpMetadata::get_input_stream_infos(const std::string &network_name) const
{
    auto input_layer_infos = get_input_layer_infos(network_name);
    CHECK_EXPECTED(input_layer_infos);

    return convert_layer_infos_to_stream_infos(input_layer_infos.value());
}

Expected<std::vector<hailo_stream_info_t>> CoreOpMetadata::get_output_stream_infos(const std::string &network_name) const
{
    auto output_layer_infos = get_output_layer_infos(network_name);
    CHECK_EXPECTED(output_layer_infos);

    return convert_layer_infos_to_stream_infos(output_layer_infos.value());
}

Expected<std::vector<hailo_stream_info_t>> CoreOpMetadata::get_all_stream_infos(const std::string &network_name) const
{
    auto input_stream_infos = get_input_stream_infos(network_name);
    CHECK_EXPECTED(input_stream_infos);

    auto output_stream_infos = get_output_stream_infos(network_name);
    CHECK_EXPECTED(output_stream_infos);

    std::vector<hailo_stream_info_t> res;
    res.reserve(input_stream_infos->size() + output_stream_infos->size());
    res.insert(res.end(), input_stream_infos->begin(), input_stream_infos->end());
    res.insert(res.end(), output_stream_infos->begin(), output_stream_infos->end());

    return res;
}

Expected<std::vector<hailo_vstream_info_t>> CoreOpMetadata::get_input_vstream_infos(const std::string &network_name) const
{
    auto input_layer_infos = get_input_layer_infos(network_name);
    CHECK_EXPECTED(input_layer_infos);

    return convert_layer_infos_to_vstream_infos(input_layer_infos.value());
}

Expected<std::vector<hailo_vstream_info_t>> CoreOpMetadata::get_output_vstream_infos(const std::string &network_name) const
{
    std::vector<hailo_vstream_info_t> res;
    if (m_supported_features.hailo_net_flow) {
        res = m_output_vstreams_infos;
        return res;
    }
    auto expected_output_layer_infos = get_output_layer_infos(network_name);
    CHECK_EXPECTED(expected_output_layer_infos);
    auto output_layer_infos = expected_output_layer_infos.release();

    res = convert_layer_infos_to_vstream_infos(output_layer_infos);

    hailo_status status = HAILO_SUCCESS;
    std::sort(res.begin(), res.end(),
        [this, &status](const auto &info1, const auto &info2)
    {
        const auto index1 = std::find(m_sorted_output_names.begin(), m_sorted_output_names.end(), std::string(info1.name));
        const auto index2 = std::find(m_sorted_output_names.begin(), m_sorted_output_names.end(), std::string(info2.name));

        if (m_sorted_output_names.end() == index1) {
            LOGGER__ERROR("Stream {} not found in sorted output names", info1.name);
            status = HAILO_INTERNAL_FAILURE;
            return false;
        }

        if (m_sorted_output_names.end() == index2) {
            LOGGER__ERROR("Stream {} not found in sorted output names", info2.name);
            status = HAILO_INTERNAL_FAILURE;
            return false;
        }

        return index1 < index2;
    });
    CHECK_SUCCESS_AS_EXPECTED(status);

    return res;
}

Expected<std::vector<hailo_vstream_info_t>> CoreOpMetadata::get_all_vstream_infos(const std::string &network_name) const
{
    auto input_vstream_infos = get_input_vstream_infos(network_name);
    CHECK_EXPECTED(input_vstream_infos);

    auto output_vstream_infos = get_output_vstream_infos(network_name);
    CHECK_EXPECTED(output_vstream_infos);

    std::vector<hailo_vstream_info_t> res;
    res.reserve(input_vstream_infos->size() + output_vstream_infos->size());
    res.insert(res.end(), input_vstream_infos->begin(), input_vstream_infos->end());
    res.insert(res.end(), output_vstream_infos->begin(), output_vstream_infos->end());

    return res;
}

Expected<std::vector<std::string>> CoreOpMetadata::get_vstream_names_from_stream_name(const std::string &stream_name) const
{
    std::vector<std::string> results;
    for (auto &layer_info : get_all_layer_infos()) {
        if (stream_name == layer_info.name) {
            if (layer_info.is_defused_nms) {
                return std::vector<std::string> (1, layer_info.fused_nms_layer[0].name);
            } else if (layer_info.is_mux) {
                return get_demuxes_names(layer_info);
            } else {
                return std::vector<std::string> (1, layer_info.name);
            }
        }
    }
    return make_unexpected(HAILO_NOT_FOUND);
}

Expected<std::vector<std::string>> CoreOpMetadata::get_stream_names_from_vstream_name(const std::string &vstream_name) const
{
    std::vector<std::string> results;
    for (auto &layer_info : get_all_layer_infos()) {
        if (layer_info.is_mux) {
            if (is_edge_under_mux(layer_info, vstream_name)) {
                // vstream_name is a demux of the layer info
                results.push_back(layer_info.name);
            }
        } else if (layer_info.is_defused_nms) {
            if (vstream_name == layer_info.fused_nms_layer[0].name) {
                // vstream_name is the fused-layer of the layer info
                results.push_back(layer_info.name);
            }
        } else if (m_supported_features.hailo_net_flow && layer_info.direction == HAILO_D2H_STREAM) {
            results.push_back(layer_info.name);
        } else if (vstream_name == layer_info.name) {
            // vstream_name is a regular stream
            results.push_back(layer_info.name);
        }
    }
    CHECK_AS_EXPECTED(0 < results.size(), HAILO_NOT_FOUND, "Did not found vstream {}", vstream_name);
    return results;
}

std::vector<hailo_stream_info_t> CoreOpMetadata::convert_layer_infos_to_stream_infos(const std::vector<LayerInfo> &layer_infos) const
{
    std::vector<hailo_stream_info_t> res;
    for (auto &layer_info : layer_infos) {
        res.push_back(LayerInfoUtils::get_stream_info_from_layer_info(layer_info));
    }
    return res;
}

std::vector<hailo_vstream_info_t> CoreOpMetadata::convert_layer_infos_to_vstream_infos(const std::vector<LayerInfo> &layer_infos) const
{
    std::vector<hailo_vstream_info_t> res;
    for (auto &layer_info : layer_infos) {
        auto vstream_infos = LayerInfoUtils::get_vstream_infos_from_layer_info(layer_info);
        for (const auto &vstream_info : vstream_infos) {
            // In case of fused nms layers, several LayerInfos will contain data about the same fused layer
            if (!LayerInfoUtils::vstream_info_already_in_vector(res, vstream_info.name)) {
                res.push_back(vstream_info);
            }
        }
    }
    return res;
}

Expected<std::vector<hailo_network_info_t>> CoreOpMetadata::get_network_infos() const
{
    std::vector<hailo_network_info_t> network_infos;
    network_infos.reserve(m_sorted_network_names.size());
    for (auto const &network_name : m_sorted_network_names) {
        hailo_network_info_t network_info = {};
        CHECK_AS_EXPECTED(HAILO_MAX_NETWORK_NAME_SIZE >= (network_name.length() + 1), HAILO_INTERNAL_FAILURE,
            "The network '{}' has a too long name (max is HAILO_MAX_NETWORK_NAME_SIZE)", network_name);  
        memcpy(network_info.name, network_name.c_str(), network_name.length() + 1);

        network_infos.push_back(network_info);
    }

    return network_infos;
}

size_t CoreOpMetadata::get_contexts_count() 
{
    return (m_dynamic_contexts.size() + CONTROL_PROTOCOL__CONTEXT_SWITCH_NUMBER_OF_NON_DYNAMIC_CONTEXTS);
}

Expected<size_t> CoreOpMetadata::get_total_transfer_size()
{
    size_t total_transfer_size = 0;
    for (const auto &dynamic_context : m_dynamic_contexts) {
        auto context_size = dynamic_context.get_context_transfer_size();
        CHECK_EXPECTED(context_size);
        total_transfer_size += context_size.release();
    }
    return total_transfer_size;
}

Expected<CoreOpMetadata> CoreOpMetadataPerArch::get_metadata(uint32_t partial_clusters_layout_bitmap)
{
    if (PARTIAL_CLUSTERS_LAYOUT_IGNORE == partial_clusters_layout_bitmap) {
        // Passing PARTIAL_CLUSTERS_LAYOUT_IGNORE is magic for getting one of the metadata
        assert(0 != m_metadata_per_arch.size());
        auto result = m_metadata_per_arch.begin()->second;
        return result;
    }
    if (contains(m_metadata_per_arch, partial_clusters_layout_bitmap)) {
        auto result = m_metadata_per_arch[partial_clusters_layout_bitmap];
        return result;
    }
    LOGGER__ERROR("CoreOpPerArch does not contain metadata for partial_clusters_layout_bitmap {}", partial_clusters_layout_bitmap);
    return make_unexpected(HAILO_INTERNAL_FAILURE);
}

void CoreOpMetadataPerArch::add_metadata(const CoreOpMetadata &metadata, uint32_t partial_clusters_layout_bitmap)
{
    m_metadata_per_arch[partial_clusters_layout_bitmap] = metadata;
}

} /* namespace hailort */