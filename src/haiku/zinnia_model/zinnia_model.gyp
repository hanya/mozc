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
    'relative_dir': 'haiku/zinnia_model',
    'gen_out_dir': '<(SHARED_INTERMEDIATE_DIR)/<(relative_dir)',
    'out_dir': '<(PRODUCT_DIR)',
    'zinnia_tomoe_dir%': '../../../../zinnia-tomoe-0.6.0-20080911',
  },
  'targets': [
    {
      'target_name': 'zinnia_convert',
      'type': 'executable',
      'sources': [
        '<(third_party_dir)/zinnia/zinnia/zinnia_convert.cpp',
      ],
      'dependencies': [
        '../../handwriting/zinnia.gyp:zinnia',
      ],
    },
    {
      'target_name': 'zinnia_model',
      'type': 'none',
      'dependencies': [
        'zinnia_convert',
      ],
      'actions': [
        {
          'action_name': 'make_zinnia_model',
          'inputs': [
            '<(zinnia_tomoe_dir)/handwriting-ja.model.txt',
          ],
          'outputs': [
            '<(out_dir)/handwriting-ja.model',
          ],
          'action': [
            'python', 'make_model.py',
            '--prog', '<(out_dir)/zinnia_convert',
            '--outpath', '<@(_outputs)',
            '--inputpath', '<@(_inputs)',
          ],
        },
      ],
    },
  ],
  
}
