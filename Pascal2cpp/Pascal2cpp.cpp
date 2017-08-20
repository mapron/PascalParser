/*
 * Copyright (C) 2017 Smirnov Vladimir mapron1@gmail.com
 * Source code licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 or in file COPYING-APACHE-2.0.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.h
 */

#include <CompilerFrontend.h>
#include <ast.h>

#include <QCoreApplication>
#include <QFile>

#include <iostream>

// usage: <input.pas> <output.cpp>
int main(int argc, char *argv[])
{
	QCoreApplication application( argc, argv );
	auto args = application.arguments();
	if (args.size() < 3)
		return 1;

	QFile input(args[1]);
	if (!input.open(QIODevice::ReadOnly))
		return 1;

	QString pascalSource = QString::fromUtf8(input.readAll()), cSource;

	CompilerFrontend compiler;
	if (!compiler.pascal2c(pascalSource, cSource))
	{
		for (auto & message : compiler.messages()._messages)
		{
			std::cerr << message.toString().toUtf8().constData() << std::endl;
		}
	}

	QFile output(args[2]);
	if (!output.open(QIODevice::WriteOnly))
		return 1;

	output.write(cSource.toUtf8());

	return 0;
}
