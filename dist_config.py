package = 'tilde'
excludesrc = '/(Makefile|TODO.*|SciTE.*|run\.sh|test\.c)$'
auxsources = [ 'src/.objects/*.bytes' ]

def get_replacements(mkdist):
	return [
		{
			'tag': '<VERSION>',
			'replacement': mkdist.version
		},
		{
			'tag': '<DATE>',
			'replacement': mkdist.date.strftime("%d-%m-%Y")
		},
		{
			'tag': '<OBJECTS>',
			'replacement': " ".join(mkdist.sources_to_objects(mkdist.sources, '\.cc$', '.o')),
			'files': [ 'Makefile.in' ]
		}
	]
