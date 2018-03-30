# Copyright 2010-2018, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

{
  'variables': {
    'relative_dir': 'haiku/haiku_gui',
    'gen_out_dir': '<(SHARED_INTERMEDIATE_DIR)/<(relative_dir)',
    'gen_out_gui_dir': '<(SHARED_INTERMEDIATE_DIR)/gui',
    'out_dir': '<(PRODUCT_DIR)',
  },
  'targets': [
    {
      'target_name': 'config_dialog_lib',
      'type': 'static_library',
      'variables': {
        'subdir': 'config_dialog',
      },
      'sources': [
        '<(subdir)/config_dialog.cc',
        '<(subdir)/keybinding_editor.cc',
      ],
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:config_file_stream',
        '../../client/client.gyp:client',
        '../../composer/composer.gyp:key_parser',
        '../../config/config.gyp:config_handler',
        '../../config/config.gyp:stats_config_util',
        '../../protocol/protocol.gyp:commands_proto',
        '../../protocol/protocol.gyp:config_proto',
        '../../session/session_base.gyp:keymap',
      ],
      'include_dirs': [
        '/boot/system/develop/headers/private/interface',
      ],
    },
    {
      'target_name': 'dictionary_tool_lib',
      'type': 'static_library',
      'variables': {
        'subdir': 'dictionary_tool',
      },
      'sources': [
        '<(subdir)/dictionary_tool.cc',
        '<(subdir)/dictionary_grid.cc',
        '<(subdir)/import_dialog.cc',
        '<(subdir)/find_dialog.cc',
        '<(subdir)/dictionary_list.cc',
      ],
      'dependencies': [
        '../../base/base.gyp:base',
        '../../client/client.gyp:client',
        '../../data_manager/data_manager.gyp:pos_list_provider',
        '../../dictionary/dictionary_base.gyp:pos_matcher',
        '../../dictionary/dictionary_base.gyp:user_dictionary',
        '../../protocol/protocol.gyp:commands_proto',
        '../../protocol/protocol.gyp:user_dictionary_storage_proto',
        'encoding_util',
      ],
    },
    {
      'target_name': 'word_register_dialog_lib',
      'type': 'static_library',
      'sources': [
        'word_register_dialog/word_register_dialog.cc',
      ],
      'dependencies': [
        '../../base/base.gyp:base',
        '../../client/client.gyp:client',
        '../../data_manager/data_manager.gyp:pos_list_provider',
        '../../dictionary/dictionary_base.gyp:pos_matcher',
        '../../dictionary/dictionary_base.gyp:user_dictionary',
        '../../protocol/protocol.gyp:commands_proto',
        '../../protocol/protocol.gyp:user_dictionary_storage_proto',
      ],
    },
    {
      'target_name': 'error_message_dialog_lib',
      'type': 'static_library',
      'sources': [
        'error_message_dialog/error_message_dialog.cc',
      ],
    },
    {
      'target_name': 'about_dialog_lib',
      'type': 'static_library',
      'sources': [
        'about_dialog/about_dialog.cc',
      ],
    },
    {
      'target_name': 'mozc_tool_base_lib',
      'type': 'static_library',
      'sources': [
        'base/compatible.cc',
        'base/key_filter.cc',
        'base/grid_view.cc',
        'base/mozc_tool_app.cc',
        'base/cstring_view.cc',
      ],
      'include_dirs': [
        '/boot/system/develop/headers/private/interface',
      ],
    },
    {
      'target_name': 'character_pad_lib',
      'type': 'static_library',
      'sources': [
        'character_pad/character_list.cc',
        'character_pad/character_palette.cc',
        'character_pad/character_window.cc',
        'character_pad/hand_writing.cc',
        'character_pad/tooltip_window.cc',
        'character_pad/unicode_util.cc',
        '<(gen_out_gui_dir)/character_pad/data/cp932_map.h',
        '<(gen_out_gui_dir)/character_pad/data/local_character_map.h',
        '<(gen_out_gui_dir)/character_pad/data/unicode_blocks.h',
        '<(gen_out_gui_dir)/character_pad/data/unicode_data.h',
        '<(gen_out_gui_dir)/character_pad/data/unihan_data.h',
      ],
      'dependencies': [
        '../../base/base.gyp:base',
        '../../handwriting/handwriting.gyp:handwriting_manager',
        '../../handwriting/handwriting.gyp:zinnia_handwriting',
        '../../gui/gui.gyp:gen_character_pad_files',
        '../../gui/gui.gyp:gen_character_pad_cp932_data',
        '../../gui/gui.gyp:gen_character_pad_data',
      ],
    },
    
    {
      'target_name': 'mozc_tool_haiku',
      'type': 'executable',
      'dependencies': [
        'mozc_tool_no_resource',
        'mozc_tool_rsrc',
        'mozc_tool_catalog',
      ],
      'actions': [
        {
          'action_name': 'mozc_tool_merge_rsrc',
          'inputs': [
            '<(out_dir)/mozc_tool_no_resource',
          ],
          'outputs': [
            '<(out_dir)/mozc_tool_haiku',
          ],
          'action': [
            'python', '../input_method/tools/xres.py',
            '--outpath', '<@(_outputs)',
            '--inputpath', '<@(_inputs)',
            '--rsrc', '<(gen_out_dir)/mozc_tool.rsrc',
            '--exe',
          ],
        },
      ],
    },
    {
      'target_name': 'mozc_tool_no_resource',
      'type': 'executable',
      'sources': [
        'tool/mozc_tool.cc',
      ],
      'dependencies': [
        '../../base/base.gyp:base',
        '../../protocol/protocol.gyp:config_proto',
        'config_dialog_lib',
        'word_register_dialog_lib',
        'error_message_dialog_lib',
        'about_dialog_lib',
        'mozc_tool_base_lib',
        'character_pad_lib',
        'dictionary_tool_lib',
      ],
      'link_settings': {
        'libraries': [
          '-lbe',
          '-ltranslation',
          '-llocalestub',
          '-ltracker',
          '-ltextencoding',
        ],
      },
    },
    {
      'target_name': 'mozc_tool_rsrc',
      'type': 'none',
      'actions': [
        {
          'action_name': 'mozc_tool_rdef_to_rsrc',
          'inputs': [
            './tool/mozc_tool.rdef',
          ],
          'outputs': [
            '<(gen_out_dir)/mozc_tool.rsrc',
          ],
          'action': [
            'python', '../input_method/tools/rc.py', 
            '--outpath', '<@(_outputs)', 
            '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'read_ts',
      'type': 'none',
      'variables': {
        'guidir': '../../gui',
      },
      'actions': [
        {
          'action_name': 'read_ts_action',
          'inputs': [
            '<(guidir)/about_dialog/about_dialog_ja.ts',
            '<(guidir)/character_pad/character_pad_ja.ts',
            '<(guidir)/config_dialog/config_dialog_ja.ts',
            '<(guidir)/config_dialog/keymap_ja.ts',
            '<(guidir)/dictionary_tool/dictionary_tool_ja.ts',
            '<(guidir)/error_message_dialog/error_message_dialog_ja.ts',
            '<(guidir)/word_register_dialog/word_register_dialog_ja.ts',
            './locales/about_dialog_ja.ts',
            './locales/dictionary_tool_ja.ts',
            './locales/compatible_ja.ts',
          ],
          'outputs': [
            '<(gen_out_dir)/mozc_tool_ja.catkeys',
          ],
          'action': [
            'python', 'tools/read_ts.py',
            '--src', 'locales/ja.catkeys',
            '--ts', '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'mozc_tool_pre',
      'type': 'none',
      'dependencies': [
      ],
      'actions': [
        {
          'action_name': 'mozc_tool_generate_pre',
          'inputs': [
            './base/compatible.cc',
            './about_dialog/about_dialog.cc',
            './character_pad/character_palette.cc',
            './character_pad/hand_writing.cc',
            './character_pad/character_window.cc',
            './character_pad/tooltip_window.cc',
            './config_dialog/config_dialog.cc',
            './config_dialog/keybinding_editor.cc',
            './dictionary_tool/dictionary_tool.cc',
            './dictionary_tool/find_dialog.cc',
            './dictionary_tool/import_dialog.cc',
            './error_message_dialog/error_message_dialog.cc',
            './word_register_dialog/word_register_dialog.cc',
          ],
          'outputs': [
            '<(gen_out_dir)/mozc_tool.pre',
          ],
          'action': [
            # todo, way to obtain c compiler, include path and defs
            'python', '../input_method/tools/gen_pre.py',
            '--parser', 'gcc',
            '--outpath', '<@(_outputs)',
            '--includes', './', '../../', '<(SHARED_INTERMEDIATE_DIR)/proto_out',
              './base', '/boot/system/develop/headers/private/interface',
              '<(SHARED_INTERMEDIATE_DIR)',
              '<(third_party_dir)/zinnia/zinnia',
            '--inputpath', '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'mozc_tool_catkeys',
      'type': 'none',
      'dependencies': [
        'mozc_tool_pre',
      ],
      'actions': [
        {
          'variables': {
            'app_mime_sig': 'x-vnd.Mozc-MozcTool',
          },
          'action_name': 'mozc_tool_generate_catkeys',
          'inputs': [
            '<(gen_out_dir)/mozc_tool.pre',
          ],
          'outputs': [
            './locales/en.catkeys',
          ],
          'action': [
            'python', '../input_method/tools/gen_catkeys.py',
            '--sig', '<@(app_mime_sig)',
            '--outpath', '<@(_outputs)',
            '--inputpath', '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'mozc_tool_catalog',
      'type': 'none',
      'actions': [
        {
          'variables': {
            'app_mime_sig': 'x-vnd.Mozc-MozcTool',
          },
          'action_name': 'mozc_tool_convert_catalogs',
          'inputs': [
            './locales/en.catkeys',
            './locales/ja.catkeys',
          ],
          'outputs': [
            '<(out_dir)/<@(app_mime_sig)',
          ],
          'action': [
            'python', '../input_method/tools/gen_catalogs.py',
            '--sig', '<@(app_mime_sig)',
            '--outpath', '<@(_outputs)',
            '--inputpath', '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'encoding_util',
      'type': 'static_library',
      'sources': [
        '../../gui/base/encoding_util.cc',
      ],
      'dependencies': [
        '../../base/base.gyp:base_core',
      ],
    },
  ],
  
}
