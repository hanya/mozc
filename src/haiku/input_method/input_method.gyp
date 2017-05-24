# Copyright 2010-2016, Google Inc.
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
    'relative_dir': 'haiku/input_method',
    'gen_out_dir': '<(SHARED_INTERMEDIATE_DIR)/<(relative_dir)',
    'out_dir': '<(PRODUCT_DIR)',
  },
  'targets': [
    {
      'target_name': 'input_method_lib',
      'type': 'static_library',
      'sources': [
        'bar.cc',
        'candidate_window.cc',
        'engine.cc',
        'indicator.cc',
        'looper.cc',
      ],
      'dependencies': [
        '../../base/base.gyp:base',
        '../../client/client.gyp:client',
        '../../config/config.gyp:config_handler',
        '../../ipc/ipc.gyp:ipc',
        '../../protocol/protocol.gyp:commands_proto',
        '../../protocol/protocol.gyp:config_proto',
      ],
    },
    {
      'target_name': 'mozc_no_resource',
      #'type': 'shared_library',
      'type': 'executable',
      'sources': [
        'method.cc',
      ],
      'dependencies': [
        'input_method_lib',
      ],
      'link_settings': {
        'libraries': [
          '-lbe',
          '-ltranslation',
          '-llocalestub',
          '/boot/system/servers/input_server',
        ],
      },
    },
    {
      'target_name': 'mozc',
      'type': 'none',
      'dependencies': [
        'mozc_no_resource',
        'input_method_rsrc',
        'catalogs',
      ],
      'actions': [
        {
          'action_name': 'merge_rsrc',
          'inputs': [
            '<(out_dir)/mozc_no_resource',
          ],
          'outputs': [
            '<(out_dir)/mozc',
          ],
          'action': [
            'python', 'tools/xres.py',
            '--outpath', '<@(_outputs)',
            '--inputpath', '<@(_inputs)',
            '--rsrc', '<(gen_out_dir)/input_method.rsrc',
          ],
        },
      ],
    },
    {
      'target_name': 'input_method_rsrc',
      'type': 'none',
      'actions': [
        {
          'action_name': 'rdef_to_rsrc',
          'inputs': [
            './input_method.rdef',
          ],
          'outputs': [
            '<(gen_out_dir)/input_method.rsrc',
          ],
          'action': [
            'python', 'tools/rc.py', 
            '--outpath', '<@(_outputs)', 
            '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'input_method_pre',
      'type': 'none',
      'actions': [
        {
          'action_name': 'generate_pre',
          'inputs': [
            './bar.cc',
            './looper.cc',
          ],
          'outputs': [
            '<(gen_out_dir)/input_method.pre',
          ],
          'action': [
            # todo, way to obtain c compiler, include path and defs
            'python', 'tools/gen_pre.py',
            '--parser', 'gcc',
            '--outpath', '<@(_outputs)',
            '--includes', './', '../../', '<(SHARED_INTERMEDIATE_DIR)/proto_out',
            '--inputpath', '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'catkeys',
      'type': 'none',
      'dependencies': [
        'input_method_pre',
      ],
      'actions': [
        {
          'variables': {
            'app_mime_sig': 'x-vnd.Mozc-InputMethod',
          },
          'action_name': 'generate_catkeys',
          'inputs': [
            '<(gen_out_dir)/input_method.pre',
          ],
          'outputs': [
            './locales/en.catkeys',
          ],
          'action': [
            'python', 'tools/gen_catkeys.py',
            '--sig', '<@(app_mime_sig)',
            '--outpath', '<@(_outputs)',
            '--inputpath', '<@(_inputs)',
          ],
        },
      ],
    },
    {
      'target_name': 'catalogs',
      'type': 'none',
      'actions': [
        {
          'variables': {
            'app_mime_sig': 'x-vnd.Mozc-InputMethod',
          },
          'action_name': 'convert_catalogs',
          'inputs': [
            './locales/en.catkeys',
            './locales/ja.catkeys',
          ],
          'outputs': [
            '<(out_dir)/<@(app_mime_sig)',
          ],
          'action': [
            'python', 'tools/gen_catalogs.py',
            '--sig', '<@(app_mime_sig)',
            '--outpath', '<@(_outputs)',
            '--inputpath', '<@(_inputs)',
          ],
        },
      ],
    },
  ],
  
}
